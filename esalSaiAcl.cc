/**
 * @file      esalSaiTag.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface.
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#include "headers/esalSaiUtils.h"

#include <iostream>

#include <string>
#include <cinttypes>
#include <map>

#include "esal_vendor_api/esal_vendor_api.h"

struct portVlanTransMap {
    uint16_t portid; 
    vendor_vlan_translation_t trans;
    sai_object_id_t attrSai; 
};

static std::vector<portVlanTransMap> ingressPortTransMap;
static std::vector<portVlanTransMap> egressPortTransMap;
static std::map<uint16_t, sai_object_id_t> portIngressAcl;
static std::map<uint16_t, sai_object_id_t> portEgressAcl;
static std::mutex aclMutex;

extern "C" {
static sai_object_id_t aclTableBpduTrap;
static sai_object_id_t aclEntryBpduTrap;
static sai_mac_t customBpduMac = {0x01, 0x80, 0xC2, 0x00, 0x00, 0xFF};
static std::vector<sai_object_id_t> bpduEnablePorts;
static void buildACLTable(uint32_t stage, std::vector<sai_attribute_t> &attributes){
#ifndef UTS

    sai_attribute_t attr; 

    // Define the stage. 
    //
    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    attr.value.u32 = stage;
    attributes.push_back(attr);

    // Defines the types of actions
    //
    const int actTabSize = 1;
    int32_t actTab[actTabSize];
    actTab[0] = SAI_ACL_ACTION_TYPE_SET_OUTER_VLAN_ID;
    sai_s32_list_t actTabList;
    actTabList.list = actTab;
    actTabList.count = actTabSize;
    attr.id = SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST;
    attr.value.s32list = actTabList;
    attributes.push_back(attr);

    // Define the packet fields to look at. 
    //
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_HAS_VLAN_TAG;
    attr.value.booldata = true;
    attributes.push_back(attr);

    // Define the packet fields to look at. 
    //
    attr.id = SAI_ACL_TABLE_ATTR_FIELD_PACKET_VLAN;
    attr.value.booldata = true;
    attributes.push_back(attr);

    // Define the packet fields to look at.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID;
    attr.value.booldata = true;
    attributes.push_back(attr);
#endif 
    
}

static void buildACLEntry(
     vendor_vlan_translation_t trans, sai_object_id_t aclTable, std::vector<sai_attribute_t> &aclAttr){

#ifndef UTS
    sai_attribute_t attr; 

    // Associate with respective table. 
    //
    attr.id = SAI_ACL_ENTRY_ATTR_TABLE_ID;
    attr.value.oid = aclTable;
    aclAttr.push_back(attr);
    
    sai_acl_field_data_t match;
    // Define the fields to match on...
    //
    match.enable = true;
    match.data.s32 = SAI_PACKET_VLAN_SINGLE_OUTER_TAG;
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_PACKET_VLAN;
    attr.value.aclfield = match;
    aclAttr.push_back(attr);

    sai_acl_field_data_t transMatch;
    transMatch.enable = true;
    transMatch.mask.u16 = 4095;
    transMatch.data.u16 = trans.oldVlan;

    // Mark the value to match one. 
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID;
    attr.value.aclfield = transMatch;
    aclAttr.push_back(attr);
    // Define the action to match on...
    //
    sai_acl_action_data_t transAction;
    transAction.enable = true;
    transAction.parameter.u16 = trans.newVlan;
    // Say what to do when there match.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID;
    attr.value.aclaction = transAction;
    aclAttr.push_back(attr);

#endif 

}

static void removeACLEntry(sai_object_id_t aclSai) {
#ifndef UTS

    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);

    if (retcode) {
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return;
    }

    retcode = saiAclApi->remove_acl_entry(aclSai); 
    if (retcode) {
        std::cout << "remove_acl fail: " << esalSaiError(retcode) << "\n";
        return;
    }
#endif 

}

int VendorSetIngressVlanTranslation(uint16_t lPort,
                                    vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;

    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;
    bool rc = saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort);

    if (!rc) {
        std::cout << "VendorSetIngressVlanTranslation failed to get pPort, "
                  << "lPort= " << lPort << std::endl;
        return ESAL_RC_FAIL;
    }
    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(aclMutex);

    // Find the port sai first.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)){
        std::cout << "esalPortTableFindSai not find port\n";
        return ESAL_RC_OK;
    }

    // Find ACL API 
    //
#ifndef UTS
    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail " \
                    " in VendorSetIngressVlanTranslation\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif 

    // Check to see if the ingress table has already been created.
    //
    sai_object_id_t aclTable = 0;
    auto aclTableFound = portIngressAcl.find(pPort); 
    if (aclTableFound != portIngressAcl.end()) {
        aclTable = aclTableFound->second; 
    } else {
        // STAGE is ingress. Build ACL Attributes. 
        //
        std::vector<sai_attribute_t> attributes; 
        buildACLTable(SAI_ACL_STAGE_INGRESS, attributes);

        // Create table and add to port ingress.
        //
#ifndef UTS
        retcode = saiAclApi->create_acl_table(
                &aclTable, esalSwitchId, attributes.size(), attributes.data());
        if (retcode) {
            std::cout << "VendorSetIngressVlanTranslation create acl fail "
                << esalSaiError(retcode) << "\n";
            return ESAL_RC_FAIL;
        }
#endif 
        portIngressAcl[pPort] = aclTable;

        // Add ACL Table to Port
        //
        esalAddAclToPort(portSai, aclTable, true); 
    }

    // Set up ACL Entry
    //
    std::vector<sai_attribute_t> aclAttr; 

    // Build ACL Entry List item
    //
    buildACLEntry(trans, aclTable, aclAttr); 

    // Create the ACL Entry. 
    //
    sai_object_id_t attrSai = 0;
#ifndef UTS
    retcode = saiAclApi->create_acl_entry(
            &attrSai, esalSwitchId, aclAttr.size(), aclAttr.data());
    if (retcode) {
        std::cout << "VendorSetIngressVlanTranslation add acl fail: "
            << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Push onto the port vlan map. 
    //
    portVlanTransMap newent;
    newent.portid = pPort; 
    newent.trans = trans;
    newent.attrSai = attrSai;
    ingressPortTransMap.push_back(newent); 

    return ESAL_RC_OK;
}

int VendorGetIngressVlanTranslation(uint16_t lPort, int *size,
        vendor_vlan_translation_t trans[]) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    uint32_t dev;
    uint32_t pPort;
    bool rc = saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort);

    if (!rc) {
        std::cout << "VendorGetIngressVlanTranslation failed to get pPort"
            << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Upon entryi to the routine, size should tell max size
    // for the trans array.i
    //  The returned value is the actual size.

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    // Upon entryi to the routine, size should tell
    // max size for the trans array.i
    // The returned value is the actual size.
    if (!size) {
        std::cout << "VendorGetIngressVlanTranslation null pointer\n";
        return ESAL_RC_FAIL;
    }

    // Iterate through array, and match on ports.  Assume that it is 
    // possible to have multiple matches but don't override the maximum. 
    //
    int maxsize = *size; 
    int curSize = 0;
    for(auto &ent : ingressPortTransMap) {
        if (ent.portid == pPort) {
            trans[curSize++] = ent.trans; 
            if (curSize == maxsize) {
                std::cout << "VendorGetIngressVlanTranslation max exc: pPort="
                          << pPort << "\n";
                return ESAL_RC_OK;
            }
        }
    }

    // Return the actual size of the table. 
    //
    *size = curSize; 
    return ESAL_RC_OK;
}

int VendorDeleteIngressVlanTranslation(uint16_t lPort,
                                       vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    int idx = 0;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorDeleteIngressVlanTranslation failed to get pPort "
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    // Iterate through the Port Trans Map, and match on three-way key 
    // of port, newVLAN, and oldVLAN. 
    //
    for(auto &ent : ingressPortTransMap) {
        if ((ent.portid == pPort) && 
            (ent.trans.newVlan == trans.newVlan) &&
            (ent.trans.oldVlan == trans.oldVlan)) {

            // Remove the ACL from the SAI. 
            //
            removeACLEntry(ent.attrSai);

            // Remove it from map translator. 
            //
            ingressPortTransMap.erase(ingressPortTransMap.begin()+idx);

            return ESAL_RC_OK;
        }
        idx++; 
    }

    std::cout << "VendorDeleteIngressVlanTranslation entry not found: pPort="
              << pPort << std::endl;
    return ESAL_RC_OK;
}

int VendorSetEgressVlanTranslation(uint16_t lPort,
                                   vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    uint32_t dev;
    uint32_t pPort;
    bool rc = saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort);

    if (!rc) {
        std::cout << "VendorSetEgressVlanTranslation failed to get pPort "
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(aclMutex);

    // Find the port sai first.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)){
        std::cout << "VendorSetEgressVlanTranslation not find port\n";
        return ESAL_RC_OK;
    }

    // Find ACL API 
    //
#ifndef UTS
    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in VendorSetIngressVlanTranslation\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    // Check to see if the ingress table has already been created.
    //
    sai_object_id_t aclTable = 0;
    auto aclTableFound = portEgressAcl.find(pPort); 
    if (aclTableFound != portEgressAcl.end()) {
        aclTable = aclTableFound->second; 
    } else {
        std::vector<sai_attribute_t> attributes; 
        buildACLTable(SAI_ACL_STAGE_EGRESS, attributes);

#ifndef UTS
        // Create table and add to port ingress.
        //
        retcode = saiAclApi->create_acl_table(
                &aclTable, esalSwitchId, attributes.size(), attributes.data());
        if (retcode) {
            std::cout << "VendorSetIngressVlanTranslation create acl fail: "
                      << esalSaiError(retcode) << std::endl;
            return ESAL_RC_FAIL;
        }
#endif
        portEgressAcl[pPort] = aclTable;

        // Add ACL Table to Port
        //
        esalAddAclToPort(portSai, aclTable, false); 
    }

    // Set up ACL Entry
    std::vector<sai_attribute_t> aclAttr; 

    // Build ACL Entry List item
    buildACLEntry(trans, aclTable, aclAttr); 

    sai_object_id_t attrSai = 0;
#ifndef UTS
    retcode = saiAclApi->create_acl_entry(
                &attrSai, esalSwitchId, aclAttr.size(), aclAttr.data());
    if (retcode) {
        std::cout << "VendorSetEgressVlanTranslation add acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    // Push onto the port vlan map. 
    portVlanTransMap newent;
    newent.portid = pPort; 
    newent.trans = trans;
    newent.attrSai = attrSai; 
    egressPortTransMap.push_back(newent); 

    return ESAL_RC_OK;
}

int VendorGetEgressVlanTranslation(uint16_t lPort, int *size,
        vendor_vlan_translation_t trans[]) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    uint32_t dev;
    uint32_t pPort;
    bool rc = saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort);

    if (!rc) {
        std::cout << "VendorGetEgressVlanTranslation failed to get pPort "
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    // Size should tell max size for the trans array. The returned value 
    // is the actual size.
    if (!size) {
        std::cout << "VendorGetIngressVlanTranslation null pointer"
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Iterate through array, and match on ports.  Assume that it is 
    // possible to have multiple matches but don't override the maximum. 
    //
    int maxsize = *size; 
    int curSize = 0;
    for(auto &ent : egressPortTransMap) {
        if (ent.portid == pPort) {
            trans[curSize++] = ent.trans; 
            if (curSize == maxsize) {
                std::cout << "VendorGetEgressVlanTranslation max exc: pPort"
                          << pPort << std::endl;
                return ESAL_RC_OK;
            }
        }
    }
    *size = curSize; 
    return ESAL_RC_OK;
}

int VendorDeleteEgressVlanTranslation(uint16_t lPort,
        vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    uint32_t dev;
    uint32_t pPort;
    bool rc = saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort);

    if (!rc) {
        std::cout << "VendorDeleteEgressVlanTranslation failed to get pPort "
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    // Iterate through the Port Trans Map, and match on three-way key 
    // of port, newVLAN, and oldVLAN. 
    //
    int idx = 0; 
    for(auto &ent : egressPortTransMap) {
        if ((ent.portid == pPort) && 
            (ent.trans.newVlan == trans.newVlan) &&
            (ent.trans.oldVlan == trans.oldVlan)) {
            // Remove the ACL Entry from SAI. 
            removeACLEntry(ent.attrSai);

            // Erase from port map.
            egressPortTransMap.erase(egressPortTransMap.begin()+idx);

            return ESAL_RC_OK;
        }
        idx++; 
    }

    // Report that nothing was deleted, but not necessary an error condition.
    std::cout << "VendorDeleteEgressVlanTranslation entry not found: pPort="
              << pPort << std::endl;
    return ESAL_RC_OK;
}

bool esalCreateBpduTrapAcl()
{

    // Find ACL API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalEnableBpduTrapOnPort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    // Set up ACL Entry
    std::vector<sai_attribute_t> attributes;

    // define acl table
    sai_attribute_t attr;

    // Define the stage.
    //
    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    attr.value.u32 = SAI_ACL_STAGE_INGRESS;
    attributes.push_back(attr);

    // Defines the types of actions
    //
    const int actTabSize = 2;
    int32_t actTab[actTabSize];
    actTab[0] = SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION;
    actTab[1] = SAI_ACL_ENTRY_ATTR_ACTION_COUNTER;

    sai_s32_list_t actTabList;
    actTabList.list = actTab;
    actTabList.count = actTabSize;
    attr.id = SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST;
    attr.value.s32list = actTabList;
    attributes.push_back(attr);

    // Define the packet fields to look at.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
    attr.value.booldata = true;
    attributes.push_back(attr);

    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC;
    attr.value.booldata = true;
    attributes.push_back(attr);
#ifndef UTS
    // Create table and add to port ingress.
    //
    retcode = saiAclApi->create_acl_table(
        &aclTableBpduTrap, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        std::cout << "esalEnableBpduTrapOnPort create acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return false;
    }
#endif

    // Set up ACL Entry
    std::vector<sai_attribute_t> aclAttr;

    // Associate with respective table.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_TABLE_ID;
    attr.value.oid = aclTableBpduTrap;
    aclAttr.push_back(attr);

    // Match custom BPDU dst mac.
    //
    sai_acl_field_data_t match_mac;
    match_mac.enable = true;
    memcpy(match_mac.data.mac, customBpduMac, sizeof(customBpduMac));
    // exact match mac address
    memset(match_mac.mask.mac, 0xff, sizeof(sai_mac_t));


    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC;
    attr.value.aclfield = match_mac;
    aclAttr.push_back(attr);

    sai_acl_action_data_t acl_action;
    acl_action.enable = true;
    acl_action.parameter.s32 = SAI_PACKET_ACTION_TRAP;

    attr.id = SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION;
    attr.value.aclaction = acl_action;
    aclAttr.push_back(attr);


#ifndef UTS
    retcode = saiAclApi->create_acl_entry(
                &aclEntryBpduTrap, esalSwitchId, aclAttr.size(), aclAttr.data());
    if (retcode) {
        std::cout << "esalEnableBpduTrapOnPort add acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return false;
    }
#endif

    return true;
}

bool esalEnableBpduTrapOnPort(std::vector<sai_object_id_t>& portSaiList)
{
    // Add all new ports to the list
    for (auto portSai : portSaiList) {
        auto res = find(bpduEnablePorts.begin(), bpduEnablePorts.end(), portSai);
        if (res == bpduEnablePorts.end())
            bpduEnablePorts.push_back(portSai);
    }

    std::vector<sai_object_id_t> port_list;

    for (auto portSai : bpduEnablePorts) {
        port_list.push_back(portSai);
    }

    // Find ACL API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalEnableBpduTrapOnPort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    // define acl attr
    sai_attribute_t attr;

    sai_acl_field_data_t match_in_ports;
    match_in_ports.enable = true;
    match_in_ports.data.objlist.count = (uint32_t)port_list.size();
    match_in_ports.data.objlist.list = port_list.data();
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
    attr.value.aclfield = match_in_ports;


#ifndef UTS
    retcode = saiAclApi->set_acl_entry_attribute(
                aclEntryBpduTrap, &attr);
    if (retcode) {
        std::cout << "esalEnableBpduTrapOnPort add acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return false;
    }
#endif

    return true;
}

}
