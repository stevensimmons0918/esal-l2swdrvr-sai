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

struct aclTableAttributes {
    uint8_t field_out_port;
    uint8_t field_dst_ipv6;
    sai_s32_list_t* field_acl_range_type_ptr;
    uint8_t field_tos;
    uint8_t field_ether_type;
    sai_acl_stage_t acl_stage;
    uint8_t field_acl_ip_type;
    sai_s32_list_t* acl_action_type_list_ptr;
    uint8_t field_tcp_flags;
    uint8_t field_in_port;
    uint8_t field_dscp;
    uint8_t field_src_mac;
    uint8_t field_out_ports;
    uint8_t field_in_ports;
    uint8_t field_dst_ip;
    uint8_t field_l4_dst_port;
    sai_uint32_t size;
    uint8_t field_src_ipv6;
    uint8_t field_dst_mac;
    uint8_t field_tc;
    uint8_t field_icmpv6_type;
    uint8_t field_src_ip;
    uint8_t field_ip_protocol;
    uint8_t field_outer_vlan_id;
    uint8_t field_icmpv6_code;
    uint8_t field_ipv6_next_header;
    sai_s32_list_t* acl_bind_point_type_list_ptr;
    uint8_t field_l4_src_port;
    uint8_t field_icmp_type;
    uint8_t field_icmp_code;
};

struct aclCounterAttributes {
    sai_object_id_t switch_id;
    sai_object_id_t table_id;
    sai_uint64_t packets;
    sai_uint64_t bytes;
    uint8_t enable_byte_count;
    uint8_t enable_packet_count;
};
struct aclEntryAttributes {
    sai_object_id_t switch_id;
    sai_acl_field_data_t *field_out_ports;
    sai_acl_action_data_t *action_egress_samplepacket_enable;
    sai_acl_action_data_t *action_mirror_ingress;
    sai_acl_action_data_t *action_set_policer;
    uint8_t admin_state;
    sai_acl_field_data_t *field_l4_src_port;
    sai_acl_field_data_t *field_ip_protocol;
    sai_acl_field_data_t *field_l4_dst_port;
    sai_acl_field_data_t *field_dscp;
    sai_acl_field_data_t *field_ipv6_next_header;
    sai_acl_action_data_t *action_mirror_egress;
    sai_uint32_t priority;
    sai_acl_field_data_t *field_dst_mac;
    sai_acl_field_data_t *field_in_port;
    sai_acl_field_data_t *field_acl_ip_type;
    sai_acl_field_data_t *field_src_ip;
    sai_acl_field_data_t *field_tcp_flags;
    sai_acl_field_data_t *field_outer_vlan_id;
    sai_acl_field_data_t *field_dst_ip;
    sai_acl_action_data_t *action_counter;
    sai_acl_field_data_t *field_dst_ipv6;
    sai_acl_field_data_t *field_tc;
    sai_acl_field_data_t *field_tos;
    sai_object_id_t table_id;
    sai_acl_field_data_t *field_acl_range_type;
    sai_acl_field_data_t *field_icmp_type;
    sai_acl_field_data_t *field_src_ipv6;
    sai_acl_field_data_t *field_src_mac;
    sai_acl_field_data_t *field_icmp_code;
    sai_acl_field_data_t *field_ether_type;
    sai_acl_field_data_t *field_out_port;
    sai_acl_action_data_t *action_packet_action;
    sai_acl_action_data_t *action_ingress_samplepacket_enable;
    sai_acl_field_data_t *field_icmpv6_type;
    sai_acl_action_data_t *action_set_outer_vlan_id;
    sai_acl_action_data_t *action_redirect;
    sai_acl_field_data_t *field_in_ports;
    sai_acl_field_data_t *field_icmpv6_code;
};

static std::vector<portVlanTransMap> ingressPortTransMap;
static std::vector<portVlanTransMap> egressPortTransMap;
static std::map<uint16_t, sai_object_id_t> portIngressAcl;
static std::map<uint16_t, sai_object_id_t> portEgressAcl;
static std::mutex aclMutex;

extern "C" {
static sai_object_id_t aclTableBpduTrap;
#ifndef UTS
static sai_object_id_t aclEntryBpduTrap;
#endif
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
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
    attr.value.booldata = true;
    attributes.push_back(attr);

    // Define the packet fields to look at.
    //
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS;
    attr.value.booldata = true;
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
     vendor_vlan_translation_t trans, sai_object_id_t aclTable, std::vector<sai_attribute_t> &aclAttr) {

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
    std::vector<sai_object_id_t> port_list;
    port_list.push_back(portSai);
    sai_acl_field_data_t match_in_ports;
    match_in_ports.enable = true;
    match_in_ports.data.objlist.count = (uint32_t)port_list.size();
    match_in_ports.data.objlist.list = port_list.data();
    // Define in ports for ACL
    //
    sai_attribute_t attr;
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
    attr.value.aclfield = match_in_ports;
    aclAttr.push_back(attr);

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
    std::vector<sai_object_id_t> port_list;
    port_list.push_back(portSai);
    sai_acl_field_data_t match_in_ports;
    match_in_ports.enable = true;
    match_in_ports.data.objlist.count = (uint32_t)port_list.size();
    match_in_ports.data.objlist.list = port_list.data();
    // Define out ports for ACL
    //
    sai_attribute_t attr;
    attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS;
    attr.value.aclfield = match_in_ports;
    aclAttr.push_back(attr);

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

bool esalCreateBpduTrapAcl() {

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

bool esalEnableBpduTrapOnPort(std::vector<sai_object_id_t>& portSaiList) {
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
        return false;
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

bool esalCreateAclTable(aclTableAttributes aclTableAttr, sai_object_id_t& aclTableId) {
    sai_attribute_t attr;
    std::vector<sai_attribute_t> attributes;

    if (aclTableAttr.field_out_port)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT;
        attr.value.booldata = aclTableAttr.field_out_port;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_dst_ipv6 == 1)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6;
        attr.value.booldata = aclTableAttr.field_dst_ipv6;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_acl_range_type_ptr->count != 0)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_RANGE_TYPE;
        attr.value.s32list.count = aclTableAttr.field_acl_range_type_ptr->count;
        attr.value.s32list.list = aclTableAttr.field_acl_range_type_ptr->list;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_tos)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_TOS;
        attr.value.booldata = aclTableAttr.field_tos;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_ether_type)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE;
        attr.value.booldata = aclTableAttr.field_ether_type;
        attributes.push_back(attr);
    }

    attr.id = SAI_ACL_TABLE_ATTR_ACL_STAGE;
    attr.value.s32 = aclTableAttr.acl_stage;
    attributes.push_back(attr);

    if (aclTableAttr.field_acl_ip_type)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE;
        attr.value.booldata = aclTableAttr.field_acl_ip_type;
        attributes.push_back(attr);
    }

    if (aclTableAttr.acl_action_type_list_ptr->count != 0)
    {
        attr.id = SAI_ACL_TABLE_ATTR_ACL_ACTION_TYPE_LIST;
        attr.value.s32list.count = aclTableAttr.acl_action_type_list_ptr->count;
        attr.value.s32list.list = aclTableAttr.acl_action_type_list_ptr->list;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_tcp_flags)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_TCP_FLAGS;
        attr.value.booldata = aclTableAttr.field_tcp_flags;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_in_port)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_IN_PORT;
        attr.value.booldata = aclTableAttr.field_in_port;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_dscp)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_DSCP;
        attr.value.booldata = aclTableAttr.field_dscp;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_src_mac)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC;
        attr.value.booldata = aclTableAttr.field_src_mac;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_out_ports)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS;
        attr.value.booldata = aclTableAttr.field_out_ports;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_in_ports)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_IN_PORTS;
        attr.value.booldata = aclTableAttr.field_in_ports;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_dst_ip == 1)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_IP;
        attr.value.booldata = aclTableAttr.field_dst_ip;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_l4_dst_port)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT;
        attr.value.booldata = aclTableAttr.field_l4_dst_port;
        attributes.push_back(attr);
    }

    attr.id = SAI_ACL_TABLE_ATTR_SIZE;
    attr.value.u32 = aclTableAttr.size;
    attributes.push_back(attr);

    if (aclTableAttr.field_src_ipv6 == 1)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6;
        attr.value.booldata = aclTableAttr.field_src_ipv6;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_dst_mac)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_DST_MAC;
        attr.value.booldata = aclTableAttr.field_dst_mac;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_tc)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_TC;
        attr.value.booldata = aclTableAttr.field_tc;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_icmpv6_type)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE;
        attr.value.booldata = aclTableAttr.field_icmpv6_type;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_src_ip == 1)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_SRC_IP;
        attr.value.booldata = aclTableAttr.field_src_ip;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_ip_protocol)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL;
        attr.value.booldata = aclTableAttr.field_ip_protocol;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_outer_vlan_id)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID;
        attr.value.booldata = aclTableAttr.field_outer_vlan_id;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_icmpv6_code)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_CODE;
        attr.value.booldata = aclTableAttr.field_icmpv6_code;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_ipv6_next_header)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_IPV6_NEXT_HEADER;
        attr.value.booldata = aclTableAttr.field_ipv6_next_header;
        attributes.push_back(attr);
    }

    if (aclTableAttr.acl_bind_point_type_list_ptr->count != 0)
    {
        attr.id = SAI_ACL_TABLE_ATTR_ACL_BIND_POINT_TYPE_LIST;
        attr.value.s32list.count = aclTableAttr.acl_bind_point_type_list_ptr->count;
        attr.value.s32list.list = aclTableAttr.acl_bind_point_type_list_ptr->list;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_l4_src_port)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT;
        attr.value.booldata = aclTableAttr.field_l4_src_port;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_icmp_type)
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE;
    {
        attr.value.booldata = aclTableAttr.field_icmp_type;
        attributes.push_back(attr);
    }

    if (aclTableAttr.field_icmp_code)
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ICMP_CODE;
        attr.value.booldata = aclTableAttr.field_icmp_code;
        attributes.push_back(attr);
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
                    " in esalCreateAclTable\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return false;
    }
#endif

    // Create acl table
    //
    retcode = saiAclApi->create_acl_table(
              &aclTableId, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        std::cout << "esalCreateAclTable create acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return false;
    }

	return true;
}

bool esalRemoveAclTable(sai_object_id_t acl_table_id) {
    // Find ACL API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail " \
                    " in esalCreateAclTable\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Remove acl table
    //
    retcode = saiAclApi->remove_acl_table(acl_table_id);
    if (retcode) {
        std::cout << "esalRemoveAclTable create acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }

	return true;
}

bool esalCreateAclCounter(aclCounterAttributes aclCounterAttr, sai_object_id_t& acl_entry_oid) {
    sai_status_t retcode;
    sai_attribute_t attr;
    std::vector<sai_attribute_t> attributes;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalCreateAclCounter\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }
#endif

	attr.id = SAI_ACL_COUNTER_ATTR_TABLE_ID;
    attr.value.oid = aclCounterAttr.table_id;
    attributes.push_back(attr);

	attr.id = SAI_ACL_COUNTER_ATTR_PACKETS;
    attr.value.u64 = aclCounterAttr.packets;
    attributes.push_back(attr);

	attr.id = SAI_ACL_COUNTER_ATTR_BYTES;
    attr.value.u64 = aclCounterAttr.bytes;
    attributes.push_back(attr);

	attr.id = SAI_ACL_COUNTER_ATTR_ENABLE_BYTE_COUNT;
    attr.value.booldata = aclCounterAttr.enable_byte_count;
    attributes.push_back(attr);

	attr.id = SAI_ACL_COUNTER_ATTR_ENABLE_PACKET_COUNT;
    attr.value.booldata = aclCounterAttr.enable_packet_count;
    attributes.push_back(attr);

#ifndef UTS
    retcode = saiAclApi->create_acl_counter(&acl_entry_oid, esalSwitchId , attributes.size(),  attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "create_acl_counter fail in esalCreateAclCounter\n"));
        std::cout << "create_acl_counter fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }
#endif

	return true;
}

bool esalRemoveAclCounter(sai_object_id_t acl_counter_id) {
	sai_status_t retcode;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalRemoveAclCounter\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

    // Remove acl counter
    //
    retcode = saiAclApi->remove_acl_counter(acl_counter_id);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "remove_acl_counter Fail " \
                                    "in esalRemoveAclCounter\n"));
        std::cout << "remove_acl_counter fail: " << esalSaiError(retcode)
                    << std::endl;
        return false;
    }

#endif

	return true;
}

bool esalCreateAclEntry(aclEntryAttributes attr_acl, sai_object_id_t& acl_entry_oid) {
    sai_status_t retcode;
    sai_attribute_t attr;
    std::vector<sai_attribute_t> attributes;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalCreateAclEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }
#endif

	if(attr_acl.field_out_ports->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS;
		memcpy(&attr.value.aclfield, attr_acl.field_out_ports, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_egress_samplepacket_enable->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_EGRESS_SAMPLEPACKET_ENABLE;
		memcpy(&attr.value.aclaction, attr_acl.action_egress_samplepacket_enable, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_mirror_ingress->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS;
		memcpy(&attr.value.aclaction, attr_acl.action_mirror_ingress, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_set_policer->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER;
		memcpy(&attr.value.aclaction, attr_acl.action_set_policer, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_ADMIN_STATE;
	attr.value.booldata = attr_acl.admin_state;
    attributes.push_back(attr);

	if(attr_acl.field_l4_src_port->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT;
		memcpy(&attr.value.aclfield, attr_acl.field_l4_src_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_ip_protocol->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL;
		memcpy(&attr.value.aclfield, attr_acl.field_ip_protocol, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_l4_dst_port->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT;
		memcpy(&attr.value.aclfield, attr_acl.field_l4_dst_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_dscp->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DSCP;
		memcpy(&attr.value.aclfield, attr_acl.field_dscp, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_ipv6_next_header->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IPV6_NEXT_HEADER;
		memcpy(&attr.value.aclfield, attr_acl.field_ipv6_next_header, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_mirror_egress->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS;
		memcpy(&attr.value.aclaction, attr_acl.action_mirror_egress, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_PRIORITY;
	attr.value.u32 = attr_acl.priority;
    attributes.push_back(attr);

	if(attr_acl.field_dst_mac->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC;
		memcpy(&attr.value.aclfield, attr_acl.field_dst_mac, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_in_port->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORT;
		memcpy(&attr.value.aclfield, attr_acl.field_in_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_acl_ip_type->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE;
		memcpy(&attr.value.aclfield, attr_acl.field_acl_ip_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_src_ip->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP;
		memcpy(&attr.value.aclfield, attr_acl.field_src_ip, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_tcp_flags->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS;
		memcpy(&attr.value.aclfield, attr_acl.field_tcp_flags, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_outer_vlan_id->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID;
		memcpy(&attr.value.aclfield, attr_acl.field_outer_vlan_id, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_dst_ip->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_IP;
		memcpy(&attr.value.aclfield, attr_acl.field_dst_ip, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_counter->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_COUNTER;
		memcpy(&attr.value.aclaction, attr_acl.action_counter, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_dst_ipv6->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6;
		memcpy(&attr.value.aclfield, attr_acl.field_dst_ipv6, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_tc->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TC;
		memcpy(&attr.value.aclfield, attr_acl.field_tc, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_tos->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TOS;
		memcpy(&attr.value.aclfield, attr_acl.field_tos, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_TABLE_ID;
	attr.value.oid = attr_acl.table_id;
    attributes.push_back(attr);

	if(attr_acl.field_acl_range_type->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ACL_RANGE_TYPE;
		memcpy(&attr.value.aclfield, attr_acl.field_acl_range_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_icmp_type->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE;
		memcpy(&attr.value.aclfield, attr_acl.field_icmp_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_src_ipv6->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6;
		memcpy(&attr.value.aclfield, attr_acl.field_src_ipv6, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_src_mac->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_MAC;
		memcpy(&attr.value.aclfield, attr_acl.field_src_mac, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_icmp_code->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE;
		memcpy(&attr.value.aclfield, attr_acl.field_icmp_code, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_ether_type->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE;
		memcpy(&attr.value.aclfield, attr_acl.field_ether_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_out_port->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT;
		memcpy(&attr.value.aclfield, attr_acl.field_out_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_packet_action->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION;
		memcpy(&attr.value.aclaction, attr_acl.action_packet_action, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_ingress_samplepacket_enable->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_INGRESS_SAMPLEPACKET_ENABLE;
		memcpy(&attr.value.aclaction, attr_acl.action_ingress_samplepacket_enable, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_icmpv6_type->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE;
		memcpy(&attr.value.aclfield, attr_acl.field_icmpv6_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_set_outer_vlan_id->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID;
		memcpy(&attr.value.aclaction, attr_acl.action_set_outer_vlan_id, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.action_redirect->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT;
		memcpy(&attr.value.aclaction, attr_acl.action_redirect, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_in_ports->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
		memcpy(&attr.value.aclfield, attr_acl.field_in_ports, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attr_acl.field_icmpv6_code->enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE;
		memcpy(&attr.value.aclfield, attr_acl.field_icmpv6_code, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}

    // Create acl entry
    //
#ifndef UTS
    retcode = saiAclApi->create_acl_entry(&acl_entry_oid, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "create_acl_entry Fail " \
                                    "in esalCreateAclEntry\n"));
        std::cout << "create_acl_entry fail: " << esalSaiError(retcode)
                    << std::endl;
        return false;
    }
#endif

	return true;
}

bool esalRemoveAclEntry(sai_object_id_t acl_entry_id) {
	sai_status_t retcode;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalRemoveAclEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

    // Remove acl entry
    //
    retcode = saiAclApi->remove_acl_entry(acl_entry_id);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "remove_acl_entry Fail " \
                                    "in esalRemoveAclEntry\n"));
        std::cout << "remove_acl_entry fail: " << esalSaiError(retcode)
                    << std::endl;
        return false;
    }
#endif

	return true;
}


}
