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

int VendorSetPortNniMode(uint16_t port, vendor_nni_mode_t mode) {
    std::cout << __PRETTY_FUNCTION__ << port <<  " is NYI : FIXME " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
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


