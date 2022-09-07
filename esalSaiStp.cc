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
#include "lib/swerr.h"

#ifndef UTS
#include "sai/sai.h"
#include "sai/saistp.h"
#endif

extern "C" {

int VendorSetPortStpState(uint16_t port, vendor_stp_state_t stpState) {
    (void) stpState;
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    int rc  = ESAL_RC_OK;
    return rc;
}

int VendorGetPortStpState(uint16_t port, vendor_stp_state_t *stpState) {
    (void) stpState;
    std::cout << __PRETTY_FUNCTION__ << " " << port  << " is NYI" << std::endl;
    int rc  = ESAL_RC_OK;
    return rc;
}

}

