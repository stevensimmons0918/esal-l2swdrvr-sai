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


#include "esal_vendor_api/esal_vendor_api.h"

extern "C" {

int VendorSetPortDoubleTagMode(uint16_t port, vendor_dtag_mode mode) {
    (void) mode;
    std::cout << __PRETTY_FUNCTION__ << " " << port << " " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return ESAL_RC_OK;
}

int VendorGetPortDoubleTagMode(uint16_t port, vendor_dtag_mode *mode) {
    (void) mode;
    std::cout << __PRETTY_FUNCTION__ << " " << port << " " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
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


