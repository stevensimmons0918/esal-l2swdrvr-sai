/**
 * @file      esalSaiMc.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. Multicast issues.
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"

#include <iostream>

#ifndef UTS 
#include "sai/sai.h"
#include "sai/saistp.h"
#endif
#include "esal_vendor_api/esal_vendor_api.h"
#include "lib/swerr.h"



extern "C" {

int VendorSetPortEgress(uint16_t port,  uint16_t, const uint16_t[]) {
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    int rc  = ESAL_RC_OK;
    return rc;
}


int VendorMirrorPort(uint16_t srcPort, uint16_t dstPort) {
    std::cout << __PRETTY_FUNCTION__ << " " << srcPort << " " << dstPort  << " is NYI" << std::endl;
    int rc  = ESAL_RC_OK;
    return rc;
}

int VendorRemoveMirrorPort(uint16_t srcPort, uint16_t dstPort) {
    std::cout << __PRETTY_FUNCTION__ << " " << srcPort << " " << dstPort  << " is NYI" << std::endl;
    int rc  = ESAL_RC_OK;
    return rc;

}




}

