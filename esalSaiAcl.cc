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

#include <iostream>

#include <string>
#include <cinttypes>
#include <map>

#include "lib/swerr.h"
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

static void buildACLTable(uint32_t stage, std::vector<sai_attribute_t> &attributes){
#ifndef UTS

    sai_attribute_t attr; 

    // Define the stage. 
    //
    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    attr.value.u32 = stage;
    attributes.push_back(attr);

    // Lets try to take advantage of 4-way parallel lookup in TCAM.
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
    
    // Define the fields to match on...
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_PACKET_VLAN;
    attr.value.u32 = SAI_PACKET_VLAN_SINGLE_OUTER_TAG;
    aclAttr.push_back(attr);

    // Say what to do when there match.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID;
    attr.value.u16 = trans.newVlan;
    aclAttr.push_back(attr);

    // Mark the value to match one. 
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID;
    attr.value.u16 = trans.oldVlan;
    aclAttr.push_back(attr);

    // Also mark that it has VLAN TAG (not sure if necessary). 
    // 
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_HAS_VLAN_TAG;
    attr.value.booldata = true;
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

int VendorSetIngressVlanTranslation(uint16_t portId,
                                    vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(aclMutex);

    // Find the port sai first.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(portId, &portSai)){
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
            SWERR_FILELINE, "sai_api_query fail in VendorSetIngressVlanTranslation\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif 

    // Check to see if the ingress table has already been created.
    //
    sai_object_id_t aclTable = 0;
    auto aclTableFound = portIngressAcl.find(portId); 
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
            std::cout << "VendorSetIngressVlanTranslation create acl fail: " << esalSaiError(retcode) << "\n";
            return ESAL_RC_FAIL;
        }
#endif 
        portIngressAcl[portId] = aclTable;

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
    retcode = 
        saiAclApi->create_acl_entry(
            &attrSai, esalSwitchId, aclAttr.size(), aclAttr.data());
    if (retcode) {
        std::cout << "VendorSetIngressVlanTranslation add acl fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Push onto the port vlan map. 
    //
    portVlanTransMap newent;
    newent.portid = portId; 
    newent.trans = trans;
    newent.attrSai = attrSai;
    ingressPortTransMap.push_back(newent); 

    return ESAL_RC_OK;
}

int VendorGetIngressVlanTranslation(uint16_t portId, int *size,
                                    vendor_vlan_translation_t trans[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;

    // Upon entryi to the routine, size should tell max size for the trans array.i    //  The returned value is the actual size.
    //
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
        if (ent.portid == portId) {
            trans[curSize++] = ent.trans; 
            if (curSize == maxsize) {
                std::cout << "VendorGetIngressVlanTranslation max exc: " << portId << "\n";
                return ESAL_RC_OK;
            }
        }
    }

    // Return the actual size of the table. 
    //
    *size = curSize; 
    return ESAL_RC_OK;
}

int VendorDeleteIngressVlanTranslation(uint16_t portId,
                                       vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;
    int idx = 0;

    // Iterate through the Port Trans Map, and match on three-way key 
    // of port, newVLAN, and oldVLAN. 
    //
    for(auto &ent : ingressPortTransMap) {
        if ((ent.portid == portId) && 
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

    std::cout << "VendorDeleteIngressVlanTranslation entry not found: " << portId << "\n";
    return ESAL_RC_OK;
}

int VendorSetEgressVlanTranslation(uint16_t portId,
                                    vendor_vlan_translation_t trans) {

    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(aclMutex);

    // Find the port sai first.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(portId, &portSai)){
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
            SWERR_FILELINE, "sai_api_query fail in VendorSetIngressVlanTranslation\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Check to see if the ingress table has already been created.
    //
    sai_object_id_t aclTable = 0;
    auto aclTableFound = portEgressAcl.find(portId); 
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
            std::cout << "VendorSetIngressVlanTranslation create acl fail: " << esalSaiError(retcode) << "\n";
            return ESAL_RC_FAIL;
        }
#endif
        portEgressAcl[portId] = aclTable;

        // Add ACL Table to Port
        //
        esalAddAclToPort(portSai, aclTable, false); 
    }

    // Set up ACL Entry
    //
    std::vector<sai_attribute_t> aclAttr; 

    // Build ACL Entry List item
    //
    buildACLEntry(trans, aclTable, aclAttr); 

    sai_object_id_t attrSai = 0;
#ifndef UTS
    retcode = 
        saiAclApi->create_acl_entry(
            &attrSai, esalSwitchId, aclAttr.size(), aclAttr.data());
    if (retcode) {
        std::cout << "VendorSetEgressVlanTranslation add acl fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Push onto the port vlan map. 
    //
    portVlanTransMap newent;
    newent.portid = portId; 
    newent.trans = trans;
    newent.attrSai = attrSai; 
    egressPortTransMap.push_back(newent); 

    return ESAL_RC_OK;
}

int VendorGetEgressVlanTranslation(uint16_t portId, int *size,
                                    vendor_vlan_translation_t trans[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;

    // Size should tell max size for the trans array. The returned value 
    // is the actual size.
    //
    if (!size) {
        std::cout << "VendorGetIngressVlanTranslation null pointer\n";
        return ESAL_RC_FAIL; 
    }

    // Iterate through array, and match on ports.  Assume that it is 
    // possible to have multiple matches but don't override the maximum. 
    //
    int maxsize = *size; 
    int curSize = 0;
    for(auto &ent : egressPortTransMap) {
        if (ent.portid == portId) {
            trans[curSize++] = ent.trans; 
            if (curSize == maxsize) {
                std::cout << "VendorGetEgressVlanTranslation max exc: " << portId << "\n";
                return ESAL_RC_OK;
            }
        }
    }
    *size = curSize; 
    return ESAL_RC_OK;
}

int VendorDeleteEgressVlanTranslation(uint16_t portId,
                                       vendor_vlan_translation_t trans) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << " " << std::endl;

    // Iterate through the Port Trans Map, and match on three-way key 
    // of port, newVLAN, and oldVLAN. 
    //
    int idx = 0; 
    for(auto &ent : egressPortTransMap) {
        if ((ent.portid == portId) && 
            (ent.trans.newVlan == trans.newVlan) &&
            (ent.trans.oldVlan == trans.oldVlan)) {
          
            // Remove the ACL Entry from SAI. 
            //
            removeACLEntry(ent.attrSai);

            // Erase from port map.
            //
            egressPortTransMap.erase(egressPortTransMap.begin()+idx);

            return ESAL_RC_OK;
        }
        idx++; 
    }

    // Report that nothing was deleted, but not necessary an error condition.
    //
    std::cout << "VendorDeleteEgressVlanTranslation entry not found: " << portId << "\n";
    return ESAL_RC_OK;
}

}


