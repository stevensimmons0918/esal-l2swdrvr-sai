/**
 * @file      esalSaiStp.cc
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

#include <cinttypes>

#include <esal_vendor_api/esal_vendor_api.h>


#ifndef UTS
#include "sai/sai.h"
#include "sai/saistp.h"
#endif

extern "C" {

struct StpGroupMember{
    uint16_t portId;
    sai_object_id_t stpSai;
    sai_object_id_t bridgePortSai;
    sai_object_id_t stpPortSai;
};

const int STP_PORT_TABLE_MAXSIZE = 1024;
static int stpPortTableSize;

static std::vector<StpGroupMember> stpPortTable;

bool esalFindStpPortSaiFromPortId(sai_object_id_t portId, sai_object_id_t *stpPortSai) {
   
    // Iterate over the Stp Port.
    // 
    for (auto &stpGroupMember : stpPortTable) {
        if (stpGroupMember.portId == portId) {
            *stpPortSai = stpGroupMember.stpPortSai; 
            return true; 
        }
    }

    return false; 
}

int VendorSetPortStpState(uint16_t port, vendor_stp_state_t stpState) {
    
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    sai_status_t retcode;
    sai_stp_api_t *saiStpApi;
    sai_attribute_t attr;

    retcode = sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    attr.id = SAI_STP_PORT_ATTR_STATE;
    switch (stpState) {
    case VENDOR_STP_STATE_LEARN:
        attr.value.s32 = SAI_STP_PORT_STATE_LEARNING;
        break;

    case VENDOR_STP_STATE_FORWARD:
        attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
        break;

    case VENDOR_STP_STATE_BLOCK:
        attr.value.s32 = SAI_STP_PORT_STATE_BLOCKING;
        break;
    
    default:
        attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
        break;
    }
  
    sai_object_id_t stpPortSai;
    if (!esalFindStpPortSaiFromPortId(port, &stpPortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalFindStpPortSaiFromPortId fail VendorSetPortStpState\n"));
            std::cout << "can't find stp port object for port:" << port << "\n";
                return ESAL_RC_FAIL;    
    }
    
    retcode = saiStpApi->set_stp_port_attribute(stpPortSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
    
    return ESAL_RC_OK;
}

int VendorGetPortStpState(uint16_t port, vendor_stp_state_t *stpState) {
    
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    sai_status_t retcode;
    sai_stp_api_t *saiStpApi;
    
    retcode = sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    attr.id = SAI_STP_PORT_ATTR_STATE;
    std::vector<sai_attribute_t> attributes;
    attributes.push_back(attr); 

    sai_object_id_t stpPortSai;
    if (!esalFindStpPortSaiFromPortId(port, &stpPortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalFindStpPortSaiFromPortId fail VendorGetPortStpState\n"));
            std::cout << "can't find stp port object for port:" << port << "\n";
                return ESAL_RC_FAIL;    
    }

    retcode = saiStpApi->get_stp_port_attribute(stpPortSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    switch (attributes[0].value.s32)
    {
    case SAI_STP_PORT_STATE_LEARNING:
        *stpState = VENDOR_STP_STATE_LEARN;
        break;

    case SAI_STP_PORT_STATE_FORWARDING:
        *stpState = VENDOR_STP_STATE_FORWARD;
        break;

    case SAI_STP_PORT_STATE_BLOCKING:
        *stpState = VENDOR_STP_STATE_BLOCK;
        break;
    
    default:
        *stpState = VENDOR_STP_STATE_UNKNOWN;
        break;
    }
    
    return ESAL_RC_OK;
}

bool esalStpCreate(sai_object_id_t *defStpId) {
    // Get the STP API
    //
    sai_status_t retcode;
    sai_stp_api_t *saiStpApi;
    retcode =  sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalStpCreate\n"));
        std::cout << "sai_api_query fail" << esalSaiError(retcode) << "\n";
        return false;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;

    // Create the STP object.
    //
    retcode = saiStpApi->create_stp(defStpId, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "STP object creation fails in esalStpCreate\n"));
        std::cout << "create_stp fail" << esalSaiError(retcode) << "\n";
        return false;
    }
    return true; 
}

bool esalStpPortCreate(sai_object_id_t stpSai, sai_object_id_t bridgePortSai, sai_object_id_t *stpPortSai) {
    sai_status_t retcode;
    sai_stp_api_t *saiStpApi;
    sai_object_id_t stpPortId;
    retcode =  sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalStpPortCreate\n"));
        std::cout << "esalStpPortCreate fail" << esalSaiError(retcode) << "\n";
        return false;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_STP_PORT_ATTR_STP;
    attr.value.oid = stpSai;
    attributes.push_back(attr);
    
    attr.id = SAI_STP_PORT_ATTR_BRIDGE_PORT;
    attr.value.oid = bridgePortSai;
    attributes.push_back(attr);

    attr.id = SAI_STP_PORT_ATTR_STATE;
    attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
    attributes.push_back(attr);

    retcode = saiStpApi->create_stp_port(stpPortSai, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "STP object creation fails in esalStpPortCreate\n"));
        std::cout << "esalStpPortCreate fail" << esalSaiError(retcode) << "\n";
        return false;
    }
    // Update the stp port table in the shadow.  Then bump counter.
    //
    StpGroupMember mbr; 
    mbr.bridgePortSai = bridgePortSai;
    mbr.stpSai = stpSai;
    mbr.stpPortSai = *stpPortSai;
    if (!esalFindBridgePortId(bridgePortSai, &mbr.portId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalFindBridgePortId fail esalStpPortCreate\n"));
        std::cout << "can't find portid for bridgePortSai:" << bridgePortSai << "\n";
              return ESAL_RC_FAIL;    
    }
    stpPortTable.push_back(mbr);

    return true;   
}

}
