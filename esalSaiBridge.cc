/**
 * @file      esalSaiBridge.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. BRIDGE issues.
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"

#include <iostream>

#include <cinttypes>
#include <string>
#include <mutex>
#include <vector>
#include <map>

#ifndef UTS
#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saibridge.h"
#include "sai/saifdb.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif
#endif
#include "esal_vendor_api/esal_vendor_api.h"


extern "C" {


// APPROACH TO SEMAPHORE:
//    There are multiple threads for configuring the bridge port table,
//    as well as another thread for FDB notifications. The following algorithm is
//    used for protecting PORT-ID VLAN-ID pairs.  The following
//    assumptions are built into the mutex design:
// 
//         - Port ID, BridgePort SAI, and VLAN ID must be keys for look-up.
//         - A typical port configuration is ~128 port-vlan pairs, maximum is 1024.
//         - Cannot use operating system primitives for FDB Updates
//         - Can use operating system primitives for provisioning.
// 
//   Therefore, there will be a "C" style arrays with entry containing two
//   fields: PORT ID and SAI Object. More importantly, there is a current
//   table size attribute.  All updates are made in the shadow area, above
//   the current table size.   Then, the current table size is incremented.
//   
//   Decrement moves the the last elements from the array into the deleted
//   spots, then decrements the current size counter. 
// 
//

struct BridgeMember{
    uint16_t portId;
    uint16_t vlanId; 
    sai_object_id_t portSai;
    sai_object_id_t bridgePortSai;
};

static std::mutex bridgeMutex;

const int BRIDGE_PORT_TABLE_MAXSIZE = 1024;
static int bridgePortTableSize;
static BridgeMember bridgePortTable[BRIDGE_PORT_TABLE_MAXSIZE];

static sai_object_id_t bridgeSai = SAI_NULL_OBJECT_ID; 

bool esalFindBridgePortId(sai_object_id_t bridgePortSai, uint16_t *portId) {
    
    // Iterate over the Bridge Port.
    // 
    for (auto i = 0; i < bridgePortTableSize; i++) {
        if (bridgePortTable[i].bridgePortSai == bridgePortSai) {
            *portId = bridgePortTable[i].portId; 
            return true; 
        }
    }

    return false; 

}

bool esalFindBridgePortSaiFromPortSai(sai_object_id_t portSai, sai_object_id_t *bridgePortSai) {
   
    // Iterate over the Bridge Port.
    // 
    for (auto i = 0; i < bridgePortTableSize; i++) {
        if (bridgePortTable[i].portSai == portSai) {
            *bridgePortSai = bridgePortTable[i].bridgePortSai; 
            return true; 
        }
    }

    return false; 
}

bool esalFindBridgePortSaiFromPortId(uint16_t portId, sai_object_id_t *bridgePortSai) {
   
    // Iterate over the Bridge Port.
    // 
    for (auto i = 0; i < bridgePortTableSize; i++) {
        if (bridgePortTable[i].portId == portId) {
            *bridgePortSai = bridgePortTable[i].bridgePortSai; 
            return true; 
        }
    }

    return false; 
}

bool esalBridgeCreate(void) {

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);
    
    // Check to see if bridge already exists.
    //
    if (bridgeSai != SAI_NULL_OBJECT_ID) {
        std::cout << "bridgeSai is not assigned\n";
        return true;
    }

#ifdef UTS
    bridgeSai = 1; 
#else 
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_bridge_api_t *saiBridgeApi;
    retcode =  sai_api_query(SAI_API_BRIDGE, (void**) &saiBridgeApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalBridgeCreate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";;
        return false;
    }


    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_BRIDGE_ATTR_TYPE;
    attr.value.s32 = SAI_BRIDGE_TYPE_1Q;
    attributes.push_back(attr);

    attr.id = SAI_BRIDGE_ATTR_LEARN_DISABLE;
    attr.value.booldata = false;
    attributes.push_back(attr);

    // Create the bridge object.
    //
    retcode = 
        saiBridgeApi->create_bridge(
            &bridgeSai, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_bridge in esalBridgeCreate\n"));
        std::cout << "create_bridge fail: " << esalSaiError(retcode) << "\n";
        return false;
    }
#endif

    return true; 

}

bool esalSetDefaultBridge(sai_object_id_t defaultBridgeSai) {
    
    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);

    bridgeSai = defaultBridgeSai;
    return true;

}

bool esalBridgeRemove(void) {

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);
    
    // Check to see if VLAN bridge already exists.
    //
    if (bridgeSai == SAI_NULL_OBJECT_ID) {
        std::cout << "esalBridgeRemove - bridge does not exist\n";
        return false;
    }

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_bridge_api_t *saiBridgeApi;
    retcode =  sai_api_query(SAI_API_BRIDGE, (void**) &saiBridgeApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalBridgeRemove\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return false;
    }

    // Remove the bridge object.
    //
    retcode = saiBridgeApi->remove_bridge(bridgeSai); 
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "remove_bridge fail in esalBridgeRemove\n"));
        std::cout << "remove_bridge fail: "  << esalSaiError(retcode) << "\n";
        return false;
    }
#endif

    bridgeSai = SAI_NULL_OBJECT_ID; 
    return true; 
} 

bool esalBridgePortCreate(sai_object_id_t portSai, sai_object_id_t *bridgePortSai, uint16_t vlanId) {

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);
    
    // Check to be sure that bridge was instantiated.
    //
    if (bridgeSai == SAI_NULL_OBJECT_ID) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "no bridge sai in esalBridgePortCreate\n"));
        std::cout << "esalBridgePortCreate fail, no bridge sai\n";
        return false;
    }

    // Check to see if bridge port already exists.
    //
    for (auto i = 0; i < bridgePortTableSize; i++) {
        if ((bridgePortTable[i].portSai == portSai) && 
            (bridgePortTable[i].vlanId == vlanId)){
                return true; 
        }
    }

    // Check to see max is exceeded. 
    //
    if (bridgePortTableSize >= BRIDGE_PORT_TABLE_MAXSIZE) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "table full in esalBridgePortCreate\n"));
        std::cout << "Bridge Prt Tab Exceed:" << portSai << " " << vlanId << "\n";
        return false;
    }

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_bridge_api_t *saiBridgeApi;
    retcode =  sai_api_query(SAI_API_BRIDGE, (void**) &saiBridgeApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalBridgePortCreate\n"));
        std::cout << "sai_api_query fail" << esalSaiError(retcode) << "\n";
        return false;
    }



    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE;
    attr.value.s32 = SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW;
    attributes.push_back(attr);

    attr.id = SAI_BRIDGE_PORT_ATTR_TYPE;
    attr.value.s32 = 
        (SAI_BRIDGE_PORT_TYPE_PORT == esalHostPortId) ?
           SAI_BRIDGE_PORT_TYPE_PORT : SAI_BRIDGE_PORT_TYPE_SUB_PORT;

    attributes.push_back(attr);

    if (SAI_BRIDGE_PORT_TYPE_PORT != esalHostPortId) {
        attr.id = SAI_BRIDGE_PORT_ATTR_VLAN_ID;
        attr.value.u16 = vlanId;
        attributes.push_back(attr);
    }

    attr.id = SAI_BRIDGE_PORT_ATTR_BRIDGE_ID;
    attr.value.oid = bridgeSai;
    attributes.push_back(attr);

    attr.id = SAI_BRIDGE_PORT_ATTR_PORT_ID;
    attr.value.oid = portSai;
    attributes.push_back(attr);

    attr.id = SAI_BRIDGE_PORT_ATTR_TAGGING_MODE;
    attr.value.s32 = (portSai == esalHostPortId) ? 
        SAI_BRIDGE_PORT_TAGGING_MODE_UNTAGGED : SAI_BRIDGE_PORT_TAGGING_MODE_TAGGED;
    attributes.push_back(attr);

    // Create the bridge object.
    //
    retcode = 
        saiBridgeApi->create_bridge_port(
            bridgePortSai, esalSwitchId, attributes.size(), attributes.data());
    if (retcode && retcode != SAI_STATUS_ITEM_ALREADY_EXISTS) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_bridge_port fail in esalBridgePortCreate\n"));
        std::cout << "create_bridge_port fail: " << esalSaiError(retcode) << "\n";;
        return false;
    }
#endif

    // Update the bridge port table in the shadow.  Then bump counter.
    //
    BridgeMember &mbr = bridgePortTable[bridgePortTableSize]; 
    mbr.portSai = portSai;
    mbr.vlanId = vlanId;
    mbr.bridgePortSai = *bridgePortSai;
    if (!esalPortTableFindId(portSai, &mbr.portId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalPortTableFindId fail VendorAddPortsToVlan\n"));
        std::cout << "can't find portid for portSai:" << portSai << "\n";
              return ESAL_RC_FAIL;    
    }
    bridgePortTableSize++; 

    return true;    
}

bool esalBridgePortRemove(sai_object_id_t portSai, uint16_t vlanId) {

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);

    // Check to see if bridge port already exists.
    //
    auto idx = 0;
    for (; idx < bridgePortTableSize; idx++) {
        BridgeMember &mbr = bridgePortTable[idx]; 
        if ((mbr.portSai == portSai) && (mbr.vlanId == vlanId)) {
            break;
        }
    }

    // Check to see if found.
    //
    if (idx == bridgePortTableSize) {
        return true; 
    }

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_bridge_api_t *saiBridgeApi;
    retcode =  sai_api_query(SAI_API_BRIDGE, (void**) &saiBridgeApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalBridgePortRemove\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return false;
    }

    // Delete the member.
    //    
    retcode = saiBridgeApi->remove_bridge_port(bridgePortTable[idx].bridgePortSai); 
    if (retcode){
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "remove_bridge_port fail in esalBridgePortRemove\n"));
        std::cout << "remove_bridge_port fail: " << esalSaiError(retcode) << "\n";;
        return false; 
    }
#endif

    // Take last entry in table. 
    //
    bridgePortTable[idx] = bridgePortTable[bridgePortTableSize-1];
    bridgePortTableSize--;
 
    return true; 

}

static int setMacLearning(uint16_t portId, bool enabled) {
    
    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(bridgeMutex);

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_bridge_api_t *saiBridgeApi;
    retcode =  sai_api_query(SAI_API_BRIDGE, (void**) &saiBridgeApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in setMacLearning\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Get bridge entry.  Note, dereferencing by map::operator[] has side-effect
    // of instantiating entry if it does not exist.  In this case, this is 
    // acceptable behavior. 
    //
    for (auto idx = 0; idx < bridgePortTableSize; idx++) {
    
        // Check to see if bridge port already exists.
        //
        BridgeMember &bridgeMbr = bridgePortTable[idx]; 

        if (bridgeMbr.portId != portId) continue;

        // Create Attribute list.
        //
        sai_attribute_t attr;
        attr.id = SAI_BRIDGE_PORT_ATTR_FDB_LEARNING_MODE;
        attr.value.s32 = 
            (enabled ? SAI_BRIDGE_PORT_FDB_LEARNING_MODE_HW :
                       SAI_BRIDGE_PORT_FDB_LEARNING_MODE_DISABLE);

        retcode = saiBridgeApi->set_bridge_port_attribute(bridgeMbr.bridgePortSai, &attr); 
        if (retcode){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "set_bridge_port_attribute fail in setMacLearning\n"));
            std::cout << "set_bridge_port_attribute fail: " << esalSaiError(retcode) << "\n";
        }
    }
#endif

    return ESAL_RC_OK;
}

int VendorDisableMacLearningPerPort(uint16_t portId) {
    std::cout << __PRETTY_FUNCTION__ << portId <<  " is NYI : FIXME " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return setMacLearning(portId, false);
}

int VendorEnableMacLearningPerPort(uint16_t portId) {
    std::cout << __PRETTY_FUNCTION__ << portId <<  " is NYI : FIXME " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return setMacLearning(portId, true);
}



}


