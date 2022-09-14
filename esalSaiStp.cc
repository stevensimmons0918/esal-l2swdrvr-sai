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

// int VendorSetPortStpState(uint16_t port, vendor_stp_state_t stpState) {
//     (void) stpState;
//     std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
//     if (!useSaiFlag){
//         return ESAL_RC_OK;
//     }
//     int rc  = ESAL_RC_OK;
//     return rc;
// }

int VendorGetPortStpState(uint16_t port, vendor_stp_state_t *stpState) {
    (void) stpState;
    std::cout << __PRETTY_FUNCTION__ << " " << port  << " is NYI" << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_STP_PORT_ATTR_STP;
    attributes.push_back(attr);

    attr.id = SAI_STP_PORT_ATTR_BRIDGE_PORT;
    attributes.push_back(attr);

    attr.id = SAI_STP_PORT_ATTR_STATE;
    attr.value.u8 = stpState;
    attributes.push_back(attr);

    retcode = saiStpApi->set_stp_port_attribute(port, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
    return retcode;
}

int VendorGetPortStpState(uint16_t port, vendor_stp_state_t *stpState) {
    
    sai_status_t retcode;
    // vendor_stp_state_t* retstat = NULL;
    sai_stp_api_t *saiStpApi;
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_STP_PORT_ATTR_STATE;
    attributes.push_back(attr);

    retcode = sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    retcode = saiStpApi->get_stp_port_attribute(port, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
    int rc  = ESAL_RC_OK;
    return rc;
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
    sai_attribute_t attr;

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

