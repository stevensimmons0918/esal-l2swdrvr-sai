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
#include "esal_vendor_api/esal_vendor_api.h"
#ifndef UTS
extern "C" {
#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saivlan.h"
}
#endif

extern "C" {

int VendorSetPortDoubleTagMode(uint16_t lPort, vendor_dtag_mode mode) {
    (void) mode;
    uint32_t dev;
    uint32_t pPort;

    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << " " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    return ESAL_RC_OK;
}

int VendorGetPortDoubleTagMode(uint16_t lPort, vendor_dtag_mode *mode) {
    (void) mode;
    uint32_t dev;
    uint32_t pPort;

    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << " " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    return ESAL_RC_OK;
}

int VendorSetPortNniMode(uint16_t lPort, vendor_nni_mode_t mode) {
    std::cout << __PRETTY_FUNCTION__ << lPort <<  " is NYI : FIXME " << std::endl;
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

    switch (mode) {
        case VENDOR_NNI_MODE_UNI:
            // Set port to UNI mode.
            // In this mode port should push tag on ingress
            // regardless the tags.
            if (esalVlanAddPortTagPushPop(pPort, true, true) != ESAL_RC_OK) {
                std::cout << "VendorSetPortNniMode fail pPort: " << pPort << "\n";
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "invalid port in VendorSetPortNniMode\n"));
                return ESAL_RC_FAIL;
            }

            if (esalVlanAddPortTagPushPop(pPort, false, false) != ESAL_RC_OK) {
                std::cout << "VendorSetPortNniMode fail pPort: " << pPort << "\n";
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "invalid port in VendorSetPortNniMode\n"));
                return ESAL_RC_FAIL;
            }
            break;
        case VENDOR_NNI_MODE_NNI:
            break;
        case VENDOR_NNI_MODE_ENI:
            break;
        default:
            std::cout << "VendorSetPortNniMode fail. Wrong mode\n";
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "invalid mode in VendorSetPortNniMode\n"));
            return ESAL_RC_FAIL;
            break; 
    }

    return ESAL_RC_OK;
}

int VendorGetPortNniMode(uint16_t port, vendor_nni_mode_t *mode) {
    std::cout << __PRETTY_FUNCTION__ << port <<  " is NYI : FIXME " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return ESAL_RC_OK;
}

}


