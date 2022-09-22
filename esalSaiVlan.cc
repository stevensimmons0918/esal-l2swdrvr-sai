/**
 * @file      esalSaiVlan.cc
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

#include <cinttypes>
#include <mutex>
#include <vector>
#include <map>
#include <string>

#include <esal_vendor_api/esal_vendor_api.h>

#ifndef UTS
extern "C" {
#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saivlan.h"
}

#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif
#endif

extern "C" {

struct VlanMember{
    uint16_t portId;
    sai_object_id_t memberSai;
};

struct VlanEntry {
    sai_object_id_t vlanSai;
    std::vector<VlanMember> ports;
    uint16_t defaultPortId = 0xffff;
};

std::vector<uint16_t> tagPorts;
static std::mutex vlanMutex;

static std::map<uint16_t, VlanEntry> vlanMap; 


int VendorCreateVlan(uint16_t vlanid) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << " is NYI" << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists. It is OK condition. Otherwise,
    // it will break Esal Base if fail. 
    //
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanFound != vlanMap.end()) {
        return ESAL_RC_OK; 
    }

    // Query for VLAN API
    //
    sai_object_id_t vlanSai = 0;
#ifndef UTS
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode =  sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorCreateVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_VLAN_ATTR_VLAN_ID;
    attr.value.u16 = vlanid;
    attributes.push_back(attr); 

    attr.id = SAI_VLAN_ATTR_LEARN_DISABLE;
    attr.value.booldata = false;
    attributes.push_back(attr); 

    // Create VLAN first. 
    //   
    retcode = 
        saiVlanApi->create_vlan(
            &vlanSai, esalSwitchId, attributes.size(), attributes.data()); 
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "create_vlan fail in VendorCreateVlan\n"));
        std::cout << "create_vlan fail:" << vlanid << " " 
                  << esalSaiError(retcode) <<  "\n";
        return ESAL_RC_FAIL;
    }
#endif 

    // Insert into map. There are not member ports at this point. 
    //  
    VlanEntry entry;
    entry.vlanSai = vlanSai;
    vlanMap[vlanid] = entry;
    
    return rc;
}

int VendorDeleteVlan(uint16_t vlanid) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanFound == vlanMap.end()) {
        std::cout << "vlan_map.find vlan does not exist: " << vlanid; 
        return rc; 
    }

    // Query for VLAN API
    //
#ifndef UTS 
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode = sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorDeleteVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Remove vlan object.
    //
    VlanEntry &entry = vlanMap[vlanid];
    retcode = saiVlanApi->remove_vlan(entry.vlanSai);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "remove_vlan fail in VendorDeleteVlan\n"));
        std::cout << "remove_vlan fail:" << vlanid 
                  << " " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Remove from map. 
    //  
    vlanMap.erase(vlanFound);

    return rc;
}

int VendorAddPortsToVlan(uint16_t vlanid, uint16_t numPorts, const uint16_t ports[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanFound == vlanMap.end()) {
        std::cout << "vlan_map.find vlan does not exist: " << vlanid; 
        return rc; 
    }

#ifndef UTS
    // Query for VLAN API
    //
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode = sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorAddPortsToVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Add member vlan. 
    //
    for(uint16_t i = 0; i < numPorts; i++) {
        VlanEntry &entry = vlanMap[vlanid];

        uint32_t dev;
        uint32_t pPort;

        if (!saiUtils.GetPhysicalPortInfo(ports[i], &dev, &pPort)) {
            std::cout << "VendorAddPortsToVlan, failed to get pPort"
                << " lPort=" << pPort << std::endl;
            continue;
        }

        // Check first to see if it is already stored as port. 
        //
        bool fnd = false; 
        for(auto prt : entry.ports){
            if (pPort == prt.portId) {
                std::cout << "Member exists already: " << vlanid
                    << " " << pPort << "\n";
                fnd = true;
                break; 
            }
        }

        if (fnd) continue;

#ifndef UTS
        // Now,, add objects.
        //
        std::vector<sai_attribute_t> attributes;
        sai_attribute_t attr;

        attr.id = SAI_VLAN_MEMBER_ATTR_VLAN_ID;
        attr.value.oid = entry.vlanSai;
        attributes.push_back(attr); 

        sai_object_id_t bridgePortSai;

        if (!esalFindBridgePortSaiFromPortId(pPort, &bridgePortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "esalFindBridgePortSai fail VendorAddPortsToVlan\n"));
            std::cout << "can't find bridge port object for port:" << pPort << "\n";
            return ESAL_RC_FAIL;
        }

        attr.id = SAI_VLAN_MEMBER_ATTR_BRIDGE_PORT_ID;
        attr.value.oid = bridgePortSai;
        attributes.push_back(attr); 

        attr.id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;

        // Check to see if traffic is UNTAGGED, and then mark it 
        // that it needs a tag added.
        //
        bool mustAddTag = false;
        for(auto tagPort : tagPorts){ 
            if (tagPort == pPort){
                mustAddTag = true; 
                break;
            }
        }

        attr.value.s32 = mustAddTag ?
            SAI_VLAN_TAGGING_MODE_UNTAGGED : SAI_VLAN_TAGGING_MODE_TAGGED; 
        attributes.push_back(attr); 

        sai_object_id_t memberSai;
        retcode = 
            saiVlanApi->create_vlan_member(
                    &memberSai, esalSwitchId, attributes.size(), attributes.data()); 
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "create_vlan_member fail VendorAddPortsToVlan\n"));
            std::cout << "sai_object_id_t &vlanMbrObjId fail: " << pPort << "\n";
        } else {
            // Add first to vlan map.
            //  
            VlanMember mbr;
            mbr.portId = pPort;
            mbr.memberSai = memberSai;
            entry.ports.push_back(mbr);
        }
#endif
    }

    return rc;
}

int VendorDeletePortsFromVlan(uint16_t vlanid, uint16_t numPorts, const uint16_t ports[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanFound == vlanMap.end()) {
        std::cout << "vlan_map.find vlan does not exist: " << vlanid << "\n"; 
        return rc; 
    }

    // Query for VLAN API
    //
#ifndef UTS 
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode = sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail in VendorDeletePortsFromVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif 

    // Remove member vlan. 
    //
    for(uint16_t i = 0; i < numPorts; i++) {
        VlanEntry &entry = vlanMap[vlanid];
        bool fnd = false; 
        sai_object_id_t memberSai = 0;
        int portTabIdx = 0;
        uint32_t dev;
        uint32_t pPort;

        if (!saiUtils.GetPhysicalPortInfo(ports[i], &dev, &pPort)) {
            std::cout << "VendorDisableMacLearningPerPort, failed to get pPort"
                << " lPort=" << ports[i] << std::endl;
            continue;
        }
        for(auto prt : entry.ports){
            if (pPort == prt.portId) {
                std::cout << "Member exists already: " << vlanid << " " << pPort << "\n";
                fnd = true;
                memberSai = prt.memberSai; 
                break; 
            }
            portTabIdx++;
        }

        if (!fnd) continue; 

        // Now,, remove objects.
        //
#ifdef UTS 
        (void) memberSai;
#else
        retcode = saiVlanApi->remove_vlan_member(memberSai);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "remove_vlan_member fail VendorDeletePortsFromVlan\n"));
            std::cout << "remove_vlan_member fail\n";
        } else {
            entry.ports.erase(entry.ports.begin()+portTabIdx); 
        }
#endif
    }

    return rc;
}

int VendorGetPortsInVlan(uint16_t vlanid,
        uint16_t *numPorts, uint16_t ports[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    *numPorts = 0; 
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanFound == vlanMap.end()) {
        std::cout << "vlan_map.find vlan does not exist: " << vlanid; 
        return ESAL_RC_FAIL; 
    }

    // Copy ports 
    // 
    VlanEntry &entry = vlanMap[vlanid];
    for(auto &prt : entry.ports){
        uint32_t lPort;

        if (!saiUtils.GetLogicalPort(0, prt.portId, &lPort)) {
            std::cout << "VendorGetPortsInVlan, failed to get lPort"
                << " pPort=" << prt.portId << std::endl;
            continue;
        } else {
            ports[(*numPorts)++] = lPort;
        }
    }

    return rc;
}

int VendorSetPortDefaultVlan(uint16_t lPort, uint16_t vlanid) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorSetPortDefaultVlan, failed to get pPort"
            << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    auto vlanFound = vlanMap.find(vlanid);
    if (vlanid && (vlanFound == vlanMap.end())) {
        std::cout << "vlan_map.find vlan does not exist: " << vlanid; 
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "invalid vlan fail in VendorSetPortDefaultVlan\n"));
        return ESAL_RC_FAIL; 
    }

    // Query for VLAN API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode = sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail in VendorSetPortDefaultVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Set default vlan id.
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_PORT_VLAN_ID;
    attr.value.u16 = vlanid;

    // Look up port sai.  
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {

        std::cout << "VendorSetPortDefaultVlan fail pPort: " << pPort << "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "invalid port in VendorSetPortDefaultVlan\n"));
        return ESAL_RC_FAIL;
    }

    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "set_port_attribute in VendorSetPortDefaultVlan\n"));
        std::cout << "VendorSetPortDefaultVlan fail\n";
        return ESAL_RC_FAIL;
    }


    // Check first to see if it is already stored as port. 
    //
    VlanEntry &entry = vlanMap[vlanid];
    entry.defaultPortId = pPort; 

#endif
    return rc;
}

int VendorGetPortDefaultVlan(uint16_t lPort, uint16_t *vlanid) {
    std::cout << __PRETTY_FUNCTION__ << " " << lPort << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorGetPortDefaultVlan, failed to get pPort"
            << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }
    int rc  = ESAL_RC_OK;

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Query for VLAN API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode = sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query in VendorGetPortDefaultVlan\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Set default vlan id.
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_PORT_VLAN_ID;
    std::vector<sai_attribute_t> attributes;
    attributes.push_back(attr); 

    // Look up port sai
    //
    sai_object_id_t portSai;
    if (esalPortTableFindSai(pPort, &portSai)) {
        retcode = saiPortApi->get_port_attribute(
                portSai, attributes.size(), attributes.data()); 
        if (retcode) {
            std::cout << "get_port_attributes fail:" << esalSaiError(retcode) << "\n";
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "get_port_attribute fail in VendorGetPortDefaultVlan\n"));
            return ESAL_RC_FAIL; 
        }
        *vlanid = attributes[0].value.u16;
    }; 

#endif

    return rc;
}

int VendorDeletePortDefaultVlan(uint16_t port, uint16_t vlanid) {
    std::cout << __PRETTY_FUNCTION__ << " " << vlanid << " " << port << " is NYI" << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    return VendorSetPortDefaultVlan(port, 0);

}


// In this implementation, VendorTagPacketsOnIngress and VendorStripTagsOnEgress
// are semantically the same.  This makes sense with the expectation of the 
// following: 
//      CPU Host will not be marked as tagging.
//      LCNx and OSCx will be marked as tagging.
//
int VendorTagPacketsOnIngress(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorTagPacketsOnIngress, failed to get pPort"
            << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Query for VLAN API
    //
#ifndef UTS
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode =  sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail in VendorTagPacketsOnIngress\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    attr.id = SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE;
    attr.value.s32 = SAI_VLAN_TAGGING_MODE_TAGGED; 
#endif

    // Iterate over the VLAN Map.
    //
    for (auto it = vlanMap.begin(); it != vlanMap.end(); ++it){
        auto &entry = it->second;
        for (auto portEntry : entry.ports){
            if (portEntry.portId == pPort) {
#ifndef UTS 
                retcode = saiVlanApi->set_vlan_member_attribute(
                        portEntry.memberSai, &attr);
                if (retcode) {
                    SWERR(
                            Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                                SWERR_FILELINE, 
                                "set_vlan_member_attribute fail in VendorTagPacketsOnIngress\n"));
                    std::cout << 
                        "get_port_attributes fail: " << esalSaiError(retcode) << "\n";
                }
#endif
            }

        }
    }

    bool addTag = true; 
    for(auto tagPort : tagPorts){ 
        if (tagPort == pPort){
            addTag = false;
            break;
        }
    }

    if (addTag) {
        tagPorts.push_back(pPort);
    }

    return ESAL_RC_OK;
}

int VendorStripTagsOnEgress(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort:" << lPort << " " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return VendorTagPacketsOnIngress(lPort);
}

static int setVLANLearning(uint16_t vlanId, bool enabled) {


    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(vlanMutex);

    // Check to see if VLAN already exists.
    //
    auto vlanFound = vlanMap.find(vlanId);
    if (vlanFound == vlanMap.end()) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "vlan find fail in setVLANLearning\n"));
        std::cout << "vlan_map.find vlan does not exist: " << vlanId << "\n"; 
        return ESAL_RC_FAIL; 
    }


#ifndef UTS 
    // Query for VLAN API
    //
    sai_status_t retcode;
    sai_vlan_api_t *saiVlanApi;
    retcode = sai_api_query(SAI_API_VLAN, (void**) &saiVlanApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query fail in setVLANLearning\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Set vlan attribute object.
    //
    sai_attribute_t attr;
    attr.id = SAI_VLAN_ATTR_LEARN_DISABLE;
    attr.value.booldata = (enabled ? false : true); 

    VlanEntry &entry = vlanMap[vlanId];
    retcode = saiVlanApi->set_vlan_attribute(entry.vlanSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "set_vlan_attribute fail in setVLANLearning\n"));
        std::cout << "set_vlan_attribute fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

#endif

    return ESAL_RC_OK;
}

int VendorDisableMacLearningPerVlan(uint16_t vlanId) {
    std::cout << __PRETTY_FUNCTION__ << vlanId << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return setVLANLearning(vlanId, false);
}

int VendorEnableMacLearningPerVlan(uint16_t vlanId) {
    std::cout << __PRETTY_FUNCTION__ << vlanId << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return setVLANLearning(vlanId, true);
}



}
