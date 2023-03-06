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

#include <bits/stdint-uintn.h>
#include <iostream>
#include <iomanip>

#include <string>
#include <cinttypes>
#include <map>

#include <libconfig.h++>

#include "esal_vendor_api/esal_vendor_api.h"
#include "saitypes.h"
#include <esal_warmboot_api/esal_warmboot_api.h>

struct portVlanTransMap {
    uint16_t portid;
    vendor_vlan_translation_t trans;
    sai_object_id_t attrSai;
};

struct aclTableAttributes {
    uint8_t field_out_port;
    uint8_t field_dst_ipv6;
    sai_s32_list_t* field_acl_range_type_ptr = nullptr;
    uint8_t field_tos;
    uint8_t field_ether_type;
    sai_acl_stage_t acl_stage;
    uint8_t field_acl_ip_type;
    sai_s32_list_t* acl_action_type_list_ptr = nullptr;
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
    sai_s32_list_t* acl_bind_point_type_list_ptr = nullptr;
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
    sai_acl_field_data_t field_out_ports;
    sai_acl_action_data_t action_egress_samplepacket_enable;
    sai_acl_action_data_t action_mirror_ingress;
    sai_acl_action_data_t action_set_policer;
    uint8_t admin_state;
    sai_acl_field_data_t field_l4_src_port;
    sai_acl_field_data_t field_ip_protocol;
    sai_acl_field_data_t field_l4_dst_port;
    sai_acl_field_data_t field_dscp;
    sai_acl_field_data_t field_ipv6_next_header;
    sai_acl_action_data_t action_mirror_egress;
    sai_uint32_t priority;
    sai_acl_field_data_t field_dst_mac;
    sai_acl_field_data_t field_in_port;
    sai_acl_field_data_t field_acl_ip_type;
    sai_acl_field_data_t field_src_ip;
    sai_acl_field_data_t field_tcp_flags;
    sai_acl_field_data_t field_outer_vlan_id;
    sai_acl_field_data_t field_dst_ip;
    sai_acl_action_data_t action_counter;
    sai_acl_field_data_t field_dst_ipv6;
    sai_acl_field_data_t field_tc;
    sai_acl_field_data_t field_tos;
    sai_object_id_t table_id;
    sai_acl_field_data_t field_acl_range_type;
    sai_acl_field_data_t field_icmp_type;
    sai_acl_field_data_t field_src_ipv6;
    sai_acl_field_data_t field_src_mac;
    sai_acl_field_data_t field_icmp_code;
    sai_acl_field_data_t field_ether_type;
    sai_acl_field_data_t field_out_port;
    sai_acl_action_data_t action_packet_action;
    sai_acl_action_data_t action_ingress_samplepacket_enable;
    sai_acl_field_data_t field_icmpv6_type;
    sai_acl_action_data_t action_set_outer_vlan_id;
    sai_acl_action_data_t action_redirect;
    sai_acl_field_data_t field_in_ports;
    sai_acl_field_data_t field_icmpv6_code;
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
static void buildACLTable(uint32_t stage, std::vector<sai_attribute_t> &attributes) {
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
    if (!esalPortTableFindSai(pPort, &portSai)) {
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

#ifndef UTS
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

    if (aclTableAttr.field_acl_range_type_ptr && aclTableAttr.field_acl_range_type_ptr->count != 0)
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

    if (aclTableAttr.acl_action_type_list_ptr && aclTableAttr.acl_action_type_list_ptr->count != 0)
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

    if (aclTableAttr.acl_bind_point_type_list_ptr && aclTableAttr.acl_bind_point_type_list_ptr->count != 0)
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
    {
        attr.id = SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE;
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

    // Create acl table
    //
    retcode = saiAclApi->create_acl_table(
              &aclTableId, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        std::cout << "esalCreateAclTable create acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return false;
    }
#endif

    return true;
}

bool esalRemoveAclTable(sai_object_id_t aclTableId) {
    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    auto retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail " \
                    " in esalCreateAclTable\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Remove acl table
    //
    retcode = saiAclApi->remove_acl_table(aclTableId);
    if (retcode) {
        std::cout << "esalRemoveAclTable create acl fail: "
                  << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }

#endif
    return true;
}

bool esalCreateAclCounter(aclCounterAttributes aclCounterAttr, sai_object_id_t& aclCounterOid) {
    sai_attribute_t attr;
    std::vector<sai_attribute_t> attributes;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    auto retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
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
    retcode = saiAclApi->create_acl_counter(&aclCounterOid, esalSwitchId , attributes.size(),  attributes.data());
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

bool esalRemoveAclCounter(sai_object_id_t aclCounterId) {

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    auto retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
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
    retcode = saiAclApi->remove_acl_counter(aclCounterId);
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

bool esalCreateAclEntry(aclEntryAttributes attrAcl, sai_object_id_t& aclEntryOid) {
    sai_attribute_t attr;
    std::vector<sai_attribute_t> attributes;

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    auto retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                   SWERR_FILELINE,
                   "sai_api_query fail in esalCreateAclEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }
#endif

	if(attrAcl.field_out_ports.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS;
		memcpy(&attr.value.aclfield, &attrAcl.field_out_ports, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_egress_samplepacket_enable.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_EGRESS_SAMPLEPACKET_ENABLE;
		memcpy(&attr.value.aclaction, &attrAcl.action_egress_samplepacket_enable, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_mirror_ingress.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS;
		memcpy(&attr.value.aclaction, &attrAcl.action_mirror_ingress, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_set_policer.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_POLICER;
		memcpy(&attr.value.aclaction, &attrAcl.action_set_policer, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_ADMIN_STATE;
	attr.value.booldata = &attrAcl.admin_state;
    attributes.push_back(attr);

	if(attrAcl.field_l4_src_port.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_L4_SRC_PORT;
		memcpy(&attr.value.aclfield, &attrAcl.field_l4_src_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_ip_protocol.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IP_PROTOCOL;
		memcpy(&attr.value.aclfield, &attrAcl.field_ip_protocol, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_l4_dst_port.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_L4_DST_PORT;
		memcpy(&attr.value.aclfield, &attrAcl.field_l4_dst_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_dscp.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DSCP;
		memcpy(&attr.value.aclfield, &attrAcl.field_dscp, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_ipv6_next_header.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IPV6_NEXT_HEADER;
		memcpy(&attr.value.aclfield, &attrAcl.field_ipv6_next_header, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_mirror_egress.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_EGRESS;
		memcpy(&attr.value.aclaction, &attrAcl.action_mirror_egress, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_PRIORITY;
	attr.value.u32 = attrAcl.priority;
    attributes.push_back(attr);

	if(attrAcl.field_dst_mac.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_MAC;
		memcpy(&attr.value.aclfield, &attrAcl.field_dst_mac, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_in_port.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORT;
		memcpy(&attr.value.aclfield, &attrAcl.field_in_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_acl_ip_type.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ACL_IP_TYPE;
		memcpy(&attr.value.aclfield, &attrAcl.field_acl_ip_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_src_ip.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_IP;
		memcpy(&attr.value.aclfield, &attrAcl.field_src_ip, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_tcp_flags.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TCP_FLAGS;
		memcpy(&attr.value.aclfield, &attrAcl.field_tcp_flags, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_outer_vlan_id.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUTER_VLAN_ID;
		memcpy(&attr.value.aclfield, &attrAcl.field_outer_vlan_id, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_dst_ip.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_IP;
		memcpy(&attr.value.aclfield, &attrAcl.field_dst_ip, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_counter.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_COUNTER;
		memcpy(&attr.value.aclaction, &attrAcl.action_counter, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_dst_ipv6.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_DST_IPV6;
		memcpy(&attr.value.aclfield, &attrAcl.field_dst_ipv6, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_tc.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TC;
		memcpy(&attr.value.aclfield, &attrAcl.field_tc, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_tos.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_TOS;
		memcpy(&attr.value.aclfield, &attrAcl.field_tos, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}

	attr.id = SAI_ACL_ENTRY_ATTR_TABLE_ID;
	attr.value.oid = attrAcl.table_id;
    attributes.push_back(attr);

	if(attrAcl.field_acl_range_type.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ACL_RANGE_TYPE;
		memcpy(&attr.value.aclfield, &attrAcl.field_acl_range_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_icmp_type.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMP_TYPE;
		memcpy(&attr.value.aclfield, &attrAcl.field_icmp_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_src_ipv6.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_IPV6;
		memcpy(&attr.value.aclfield, &attrAcl.field_src_ipv6, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_src_mac.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_SRC_MAC;
		memcpy(&attr.value.aclfield, &attrAcl.field_src_mac, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_icmp_code.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMP_CODE;
		memcpy(&attr.value.aclfield, &attrAcl.field_icmp_code, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_ether_type.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ETHER_TYPE;
		memcpy(&attr.value.aclfield, &attrAcl.field_ether_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_out_port.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORT;
		memcpy(&attr.value.aclfield, &attrAcl.field_out_port, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_packet_action.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_PACKET_ACTION;
		memcpy(&attr.value.aclaction, &attrAcl.action_packet_action, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_ingress_samplepacket_enable.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_INGRESS_SAMPLEPACKET_ENABLE;
		memcpy(&attr.value.aclaction, &attrAcl.action_ingress_samplepacket_enable, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_icmpv6_type.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_TYPE;
		memcpy(&attr.value.aclfield, &attrAcl.field_icmpv6_type, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_set_outer_vlan_id.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID;
		memcpy(&attr.value.aclaction, &attrAcl.action_set_outer_vlan_id, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.action_redirect.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT;
		memcpy(&attr.value.aclaction, &attrAcl.action_redirect, sizeof(sai_acl_action_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_in_ports.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_IN_PORTS;
		memcpy(&attr.value.aclfield, &attrAcl.field_in_ports, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}
	if(attrAcl.field_icmpv6_code.enable != 0)
	{
		attr.id = SAI_ACL_ENTRY_ATTR_FIELD_ICMPV6_CODE;
		memcpy(&attr.value.aclfield, &attrAcl.field_icmpv6_code, sizeof(sai_acl_field_data_t));
        attributes.push_back(attr);
	}

    // Create acl entry
    //
#ifndef UTS
    retcode = saiAclApi->create_acl_entry(&aclEntryOid, esalSwitchId, attributes.size(), attributes.data());
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

    // Find ACL API
    //
#ifndef UTS
    sai_acl_api_t *saiAclApi;
    auto retcode =  sai_api_query(SAI_API_ACL, (void**) &saiAclApi);
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

// Drop packet with a specified src mac as default
//
bool sample_create_acl_src_mac_rule(sai_mac_t srcMac, sai_acl_stage_t stage, uint16_t portId) {
    bool status;

    sai_object_id_t aclTableOid;
    sai_object_id_t aclEntryOid;

    // Table
    //
    aclTableAttributes aclTableAttr;
    memset((void*) &aclTableAttr, 0, sizeof(aclTableAttr));

    aclTableAttr.field_src_mac = 1;
    aclTableAttr.acl_stage = stage;

    std::vector<int32_t> actTab;
    actTab.push_back(SAI_ACL_ACTION_TYPE_PACKET_ACTION);
    sai_s32_list_t actTabList;
    actTabList.count = actTab.size();
    actTabList.list = actTab.data();
    aclTableAttr.acl_action_type_list_ptr = &actTabList;

    printf("Creating acl table... ");
    status = esalCreateAclTable(aclTableAttr, aclTableOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclTableOid);

    // Entry
    //
    aclEntryAttributes aclEntryAttr;
    memset(&aclEntryAttr, 0, sizeof(aclEntryAttributes));

    aclEntryAttr.table_id = aclTableOid;

    aclEntryAttr.field_src_mac.enable = true;
    memcpy(aclEntryAttr.field_src_mac.data.mac, srcMac, sizeof(sai_mac_t));
    memset(aclEntryAttr.field_src_mac.mask.mac, 0xff, sizeof(sai_mac_t));

    aclEntryAttr.action_packet_action.enable = true;
    aclEntryAttr.action_packet_action.parameter.s32 = SAI_PACKET_ACTION_DROP;

    printf("Creating acl entry... ");
    status = esalCreateAclEntry(aclEntryAttr, aclEntryOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclEntryOid);

    // Connect table to port
    //
    sai_object_id_t portOid;
    status = esalPortTableFindSai(portId, &portOid);
    if (!status) return false;

    bool ingr;
    if (stage == SAI_ACL_STAGE_INGRESS) {
        ingr = true;
    } else if (stage == SAI_ACL_STAGE_EGRESS) {
        ingr = false;
    } else {
        return false;
    }

    printf("Connecting acl table to port... ");
    status = esalAddAclToPort(portOid, aclTableOid, ingr);
    if (!status) return false;
    printf("success!\n");

    return true;
}

// Drop packet with a specified dst mac as default
//
bool sample_create_acl_dst_mac_rule(sai_mac_t dstMac, sai_acl_stage_t stage, uint16_t portId) {
    bool status;

    sai_object_id_t aclTableOid;
    sai_object_id_t aclEntryOid;

    // Table
    //
    aclTableAttributes aclTableAttr;
    memset((void*) &aclTableAttr, 0, sizeof(aclTableAttr));

    aclTableAttr.field_dst_mac = 1;
    aclTableAttr.acl_stage = stage;

    std::vector<int32_t> actTab;
    actTab.push_back(SAI_ACL_ACTION_TYPE_PACKET_ACTION);
    sai_s32_list_t actTabList;
    actTabList.count = actTab.size();
    actTabList.list = actTab.data();
    aclTableAttr.acl_action_type_list_ptr = &actTabList;

    printf("Creating acl table... ");
    status = esalCreateAclTable(aclTableAttr, aclTableOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclTableOid);

    // Entry
    //
    aclEntryAttributes aclEntryAttr;
    memset(&aclEntryAttr, 0, sizeof(aclEntryAttributes));

    aclEntryAttr.table_id = aclTableOid;

    aclEntryAttr.field_dst_mac.enable = true;
    memcpy(aclEntryAttr.field_dst_mac.data.mac, dstMac, sizeof(sai_mac_t));
    memset(aclEntryAttr.field_dst_mac.mask.mac, 0xff, sizeof(sai_mac_t));

    aclEntryAttr.action_packet_action.enable = true;
    aclEntryAttr.action_packet_action.parameter.s32 = SAI_PACKET_ACTION_DROP;

    printf("Creating acl entry... ");
    status = esalCreateAclEntry(aclEntryAttr, aclEntryOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclEntryOid);

    // Connect table to port
    //
    sai_object_id_t portOid;
    status = esalPortTableFindSai(portId, &portOid);
    if (!status) return false;

    bool ingr;
    if (stage == SAI_ACL_STAGE_INGRESS) {
        ingr = true;
    } else if (stage == SAI_ACL_STAGE_EGRESS) {
        ingr = false;
    } else {
        return false;
    }

    printf("Connecting acl table to port... ");
    status = esalAddAclToPort(portOid, aclTableOid, ingr);
    if (!status) return false;
    printf("success!\n");

    return true;
}

// Drop packet with a specified src ip as default
//
bool sample_create_acl_src_ip_rule(sai_ip4_t srcIp, sai_acl_stage_t stage, uint16_t portId) {
    bool status;

    sai_object_id_t aclTableOid;
    sai_object_id_t aclEntryOid;

    // Table
    //
    aclTableAttributes aclTableAttr;
    memset((void*) &aclTableAttr, 0, sizeof(aclTableAttr));

    aclTableAttr.field_src_ip = 1;
    aclTableAttr.acl_stage = stage;

    std::vector<int32_t> actTab;
    actTab.push_back(SAI_ACL_ACTION_TYPE_PACKET_ACTION);
    sai_s32_list_t actTabList;
    actTabList.count = actTab.size();
    actTabList.list = actTab.data();
    aclTableAttr.acl_action_type_list_ptr = &actTabList;

    printf("Creating acl table... ");
    status = esalCreateAclTable(aclTableAttr, aclTableOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclTableOid);

    // Entry
    //
    aclEntryAttributes aclEntryAttr;
    memset(&aclEntryAttr, 0, sizeof(aclEntryAttributes));

    aclEntryAttr.table_id = aclTableOid;

    aclEntryAttr.field_src_ip.enable = true;
    aclEntryAttr.field_src_ip.data.ip4 = srcIp;
    aclEntryAttr.field_src_ip.mask.ip4 = 0xffffffff;

    aclEntryAttr.action_packet_action.enable = true;
    aclEntryAttr.action_packet_action.parameter.s32 = SAI_PACKET_ACTION_DROP;

    printf("Creating acl entry... ");
    status = esalCreateAclEntry(aclEntryAttr, aclEntryOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclEntryOid);

    // Connect table to port
    //
    sai_object_id_t portOid;
    status = esalPortTableFindSai(portId, &portOid);
    if (!status) return false;

    bool ingr;
    if (stage == SAI_ACL_STAGE_INGRESS) {
        ingr = true;
    } else if (stage == SAI_ACL_STAGE_EGRESS) {
        ingr = false;
    } else {
        return false;
    }

    printf("Connecting acl table to port... ");
    status = esalAddAclToPort(portOid, aclTableOid, ingr);
    if (!status) return false;
    printf("success!\n");

    return true;
}

// Drop packet with a specified dst ip as default
//
bool sample_create_acl_dst_ip_rule(sai_ip4_t dstIp, sai_acl_stage_t stage, uint16_t portId) {
    bool status;

    sai_object_id_t aclTableOid;
    sai_object_id_t aclEntryOid;

    // Table
    //
    aclTableAttributes aclTableAttr;
    memset((void*) &aclTableAttr, 0, sizeof(aclTableAttr));

    aclTableAttr.field_dst_ip = 1;
    aclTableAttr.acl_stage = stage;

    std::vector<int32_t> actTab;
    actTab.push_back(SAI_ACL_ACTION_TYPE_PACKET_ACTION);
    sai_s32_list_t actTabList;
    actTabList.count = actTab.size();
    actTabList.list = actTab.data();
    aclTableAttr.acl_action_type_list_ptr = &actTabList;

    printf("Creating acl table... ");
    status = esalCreateAclTable(aclTableAttr, aclTableOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclTableOid);

    // Entry
    //
    aclEntryAttributes aclEntryAttr;
    memset(&aclEntryAttr, 0, sizeof(aclEntryAttributes));

    aclEntryAttr.table_id = aclTableOid;

    aclEntryAttr.field_dst_ip.enable = true;
    aclEntryAttr.field_dst_ip.data.ip4 = dstIp;
    aclEntryAttr.field_dst_ip.mask.ip4 = 0xffffffff;

    aclEntryAttr.action_packet_action.enable = true;
    aclEntryAttr.action_packet_action.parameter.s32 = SAI_PACKET_ACTION_DROP;

    printf("Creating acl entry... ");
    status = esalCreateAclEntry(aclEntryAttr, aclEntryOid);
    if (!status) return false;
    printf("success! oid = %lX\n", aclEntryOid);

    // Connect table to port
    //
    sai_object_id_t portOid;
    status = esalPortTableFindSai(portId, &portOid);
    if (!status) return false;

    bool ingr;
    if (stage == SAI_ACL_STAGE_INGRESS) {
        ingr = true;
    } else if (stage == SAI_ACL_STAGE_EGRESS) {
        ingr = false;
    } else {
        return false;
    }

    printf("Connecting acl table to port... ");
    status = esalAddAclToPort(portOid, aclTableOid, ingr);
    if (!status) return false;
    printf("success!\n");

    return true;
}

bool run_acl_samples() {
    bool status;

    sai_mac_t srcMac = {0};
    srcMac[5] = 0x28; // 00:00:00:00:00:28

    sai_mac_t dstMac = {0};
    dstMac[5] = 0x29; // 00:00:00:00:00:29

    sai_ip4_t srcIp = __builtin_bswap32(168453130); // 10.10.100.10
    sai_ip4_t dstIp = __builtin_bswap32(168453131); // 10.10.100.11

    sai_acl_stage_t stage = SAI_ACL_STAGE_INGRESS;

    uint16_t portId = 28;

    std::cout << std::endl << "Acl test 1: drop a package with the src mac 00:00:00:00:00:28" << std::endl;
    do {std::cout << "Press enter to continue...";} while (std::cin.get() != '\n');
    status = sample_create_acl_src_mac_rule(srcMac, stage, portId);
    if (!status) return false;

    std::cout << std::endl << "Acl test 2: drop a package with the dst mac 00:00:00:00:00:29" << std::endl;
    do {std::cout << "Press enter to continue...";} while (std::cin.get() != '\n');
    status = sample_create_acl_dst_mac_rule(dstMac, stage, portId);
    if (!status) return false;

    std::cout << std::endl << "Acl test 3: drop a package with the src ipv4 10.10.100.10" << std::endl;
    do {std::cout << "Press enter to continue...";} while (std::cin.get() != '\n');
    status = sample_create_acl_src_ip_rule(srcIp, stage, portId);
    if (!status) return false;

    std::cout << std::endl << "Acl test 4: drop a package with the dst ipv4 10.10.100.11" << std::endl;
    do {std::cout << "Press enter to continue...";} while (std::cin.get() != '\n');
    status = sample_create_acl_dst_ip_rule(dstIp, stage, portId);
    if (!status) return false;

    return true;
}

static bool serializePortTransMapConfig(const std::vector<portVlanTransMap> &portTransMap, const std::string &fileName) {
    std::unique_lock<std::mutex> lock(aclMutex);

    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    libconfig::Setting &portTransMapSetting = root.add("portTransMap", libconfig::Setting::TypeList);

    for (const auto &portTrans : portTransMap) {
        libconfig::Setting &portEntry = portTransMapSetting.add(libconfig::Setting::TypeGroup);
        portEntry.add("portId", libconfig::Setting::TypeInt) = portTrans.portid;
        portEntry.add("attrSai", libconfig::Setting::TypeInt64) = static_cast<int64_t>(portTrans.attrSai);
        portEntry.add("oldVlan", libconfig::Setting::TypeInt) = portTrans.trans.oldVlan;
        portEntry.add("newVlan", libconfig::Setting::TypeInt) = portTrans.trans.newVlan;
    }

    try {
        cfg.writeFile(fileName.c_str());
        return true;
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error writing to file: " << ex.what() << std::endl;
        return false;
    }
}

static bool deserializePortTransMapConfig(std::vector<portVlanTransMap> &portTransMap, const std::string &fileName) {
    libconfig::Config cfg;
    try {
        cfg.readFile(fileName.c_str());
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error reading file: " << ex.what() << std::endl;
        return false;
    } catch (const libconfig::ParseException &ex) {
        std::cout << "Error parsing file: " << ex.what() << " at line " << ex.getLine() << std::endl;
        return false;
    }

    libconfig::Setting &portTransMapSetting = cfg.lookup("portTransMap");
    if (!portTransMapSetting.isList()) {
        std::cout << "portTransMap is not a list" << std::endl;
        return false;
    }

    portTransMap.clear();
    for (int i = 0; i < portTransMapSetting.getLength(); ++i) {
        libconfig::Setting &portTrans = portTransMapSetting[i];

        int portId, oldVlan, newVlan;
        long long attrSai;

        if (!(portTrans.lookupValue("portId", portId) &&
              portTrans.lookupValue("attrSai", attrSai) &&
              portTrans.lookupValue("oldVlan", oldVlan) &&
              portTrans.lookupValue("newVlan", newVlan))) {
            return false;
        }

        portVlanTransMap portTransEntry;
        portTransEntry.portid = static_cast<uint16_t>(portId);
        portTransEntry.attrSai = static_cast<sai_object_id_t>(attrSai);
        portTransEntry.trans.oldVlan = static_cast<uint16_t>(oldVlan);
        portTransEntry.trans.newVlan = static_cast<uint16_t>(newVlan);
        portTransMap.push_back(portTransEntry);
    }

    return true;
}

static void printVlanTranslation(const portVlanTransMap& trans) {
    std::cout << "pPortid: " << std::dec << trans.portid <<
                 ", attrSai: 0x" << std::setw(16) << std::setfill('0') << std::hex << trans.attrSai <<
                 ", oldVlan: " << std::dec << trans.trans.oldVlan <<
                 ", newVlan: " << std::dec << trans.trans.newVlan <<
                 std::endl;
}

static bool serializePortAclMap(const std::map<uint16_t, sai_object_id_t>& aclMap, const std::string& fileName) {
    std::unique_lock<std::mutex> lock(aclMutex);

    libconfig::Config cfg;
    libconfig::Setting& root = cfg.getRoot();

    libconfig::Setting& aclMapSetting = root.add("aclMap", libconfig::Setting::TypeList);

    for (const auto& acl : aclMap) {
        libconfig::Setting& aclEntry = aclMapSetting.add(libconfig::Setting::TypeGroup);
        aclEntry.add("portId", libconfig::Setting::TypeInt) = acl.first;
        aclEntry.add("aclId", libconfig::Setting::TypeInt64) = static_cast<int64_t>(acl.second);
    }

    try {
        cfg.writeFile(fileName.c_str());
        return true;
    } catch (const libconfig::FileIOException& ex) {
        std::cout << "Error writing to file: " << ex.what() << std::endl;
        return false;
    }
}

static bool deserializePortAclMap(std::map<uint16_t, sai_object_id_t>& aclMap, const std::string& fileName) {
    libconfig::Config cfg;
    try {
        cfg.readFile(fileName.c_str());
    } catch (const libconfig::FileIOException& ex) {
        std::cout << "Error reading file: " << ex.what() << std::endl;
        return false;
    } catch (const libconfig::ParseException& ex) {
        std::cout << "Error parsing file: " << ex.what() << " at line " << ex.getLine() << std::endl;
        return false;
    }

    libconfig::Setting& aclMapSetting = cfg.lookup("aclMap");
    if (!aclMapSetting.isList()) {
        std::cout << "aclMap is not a list" << std::endl;
        return false;
    }

    aclMap.clear();
    for (int i = 0; i < aclMapSetting.getLength(); ++i) {
        libconfig::Setting& acl = aclMapSetting[i];

        int portId;
        long long aclId;

        if (!(acl.lookupValue("portId", portId) && 
              acl.lookupValue("aclId", aclId))) {
            return false;
        }

        aclMap[portId] = static_cast<sai_object_id_t>(aclId);
    }

    return true;
}

static void printAcl(const uint16_t pPortNum,  const sai_object_id_t aclTableOid) {
    std::cout << "portid: " << std::dec << pPortNum <<
                 ", aclTableOid: 0x" << std::setw(16) << std::setfill('0') << std::hex << aclTableOid <<
                 std::endl;
}

bool aclWarmBootSaveHandler() {
    return (serializePortTransMapConfig(ingressPortTransMap, BACKUP_FILE_PORT_TRANS_MAP_ING) &&
            serializePortTransMapConfig(egressPortTransMap, BACKUP_FILE_PORT_TRANS_MAP_EGR) &&
            serializePortAclMap(portIngressAcl, BACKUP_FILE_PORT_ACL_ING) &&
            serializePortAclMap(portEgressAcl, BACKUP_FILE_PORT_ACL_EGR));
}

bool aclWarmBootRestoreHandler() {
    bool status = true;
    
    static std::vector<portVlanTransMap> ingressPortTransMap;
    static std::vector<portVlanTransMap> egressPortTransMap;
    static std::map<uint16_t, sai_object_id_t> portIngressAcl;
    static std::map<uint16_t, sai_object_id_t> portEgressAcl;

    status = deserializePortTransMapConfig(ingressPortTransMap, BACKUP_FILE_PORT_TRANS_MAP_ING);
    if (!status) {
        std::cout << "Error deserializing ingressPortTransMap" << std::endl;
        return false;
    }

    std::cout << "Founded ingressPortTransMap:" << std::endl;
    for (const auto& ptm_ing : ingressPortTransMap) {
        printVlanTranslation(ptm_ing);
    }

    status = deserializePortTransMapConfig(egressPortTransMap, BACKUP_FILE_PORT_TRANS_MAP_EGR);
    if (!status) {
        std::cout << "Error deserializing egressPortTransMap" << std::endl;
        return false;
    }

    std::cout << "Founded egressPortTransMap:" << std::endl;
    for (const auto& ptm_egr : egressPortTransMap) {
        printVlanTranslation(ptm_egr);
    }

    status = deserializePortAclMap(portIngressAcl, BACKUP_FILE_PORT_ACL_ING);
    if (!status) {
        std::cout << "Error deserializing portIngressAcl" << std::endl;
        return false;
    }
    
    std::cout << "Founded portIngressAcl:" << std::endl;
    for (const auto& pa_ing : portIngressAcl) {
        printAcl(pa_ing.first, pa_ing.second);
    }

    status = deserializePortAclMap(portEgressAcl, BACKUP_FILE_PORT_ACL_EGR);
    if (!status) {
        std::cout << "Error deserializing portEgressAcl" << std::endl;
        return false;
    }

    std::cout << "Founded portEgressAcl:" << std::endl;
    for (const auto& pa_egr : portEgressAcl) {
        printAcl(pa_egr.first, pa_egr.second);
    }

    ::portIngressAcl = portIngressAcl;
    ::portEgressAcl = portEgressAcl;

    std::cout << std::endl;
    std::cout << "Restore process:" << std::endl;
    if (!ingressPortTransMap.empty()) {
        for (const auto& ptm_ing : ingressPortTransMap) {
            uint32_t lport;
            if (!saiUtils.GetLogicalPort(0, ptm_ing.portid, &lport)) {
                std::cout << "aclWarmBootRestoreHandler failed to get lPort"
                          << " pPort=" << ptm_ing.portid << std::endl;
                return false;
            }
            if (VendorSetIngressVlanTranslation(lport, ptm_ing.trans) != ESAL_RC_OK) {
                std::cout << "Error creating ingress vlan translation" << std::endl;
                return false;
            }        
        }
    }

    if (!egressPortTransMap.empty()) {
        for (const auto& ptm_egr : egressPortTransMap) {
            uint32_t lport;
            if (!saiUtils.GetLogicalPort(0, ptm_egr.portid, &lport)) {
                std::cout << "aclWarmBootRestoreHandler failed to get lPort"
                          << " pPort=" << ptm_egr.portid << std::endl;
                return false;
            }
            if (VendorSetEgressVlanTranslation(lport, ptm_egr.trans) != ESAL_RC_OK) {
                std::cout << "Error creating ingress vlan translation" << std::endl;
                return false;
            }        
        }
    }

    return true;
}

void aclWarmBootCleanHandler() {
    std::unique_lock<std::mutex> lock(aclMutex);
    ingressPortTransMap.clear();
    egressPortTransMap.clear();
    portIngressAcl.clear();
    portEgressAcl.clear();
}

}
