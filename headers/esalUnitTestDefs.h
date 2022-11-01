/*
 *  esalUnitTestDefs.h 
 *
 *  Copyright (C) 2022, Fujitsu Networks Communications, Inc.
 *
 *  Brief: Global Definitions associated with ESAL SAI interface.  These
 *     are defined for the UNIT TESTs
 *
 *  Author: Steve Simmons
 */

#ifndef _UNIT_TEST_DEFS_H_
#define _UNIT_TEST_DEFS_H_

#include <stdint.h> 
#include <string>
#ifdef UTS
#include "sai.h"
#include "saitypes.h"
#include "saifdb.h"
#include "saistp.h"
struct EsalL2Filter {
    std::string name = "FOO";
    std::string mc = "00:de:ad:be:ef:00"; 
    std::string &mac() { return mc; };
    std::string mcMask = "ff:ff:ff:80:00:00"; 
    std::string &macmask() { return mcMask; }; 
    std::string &filtername() { return name; };
    bool has_mac() { return true; };
    bool has_macmask() { return true; };
    bool has_vlan() { return true; };
    bool has_vlanmask() { return true; };
    uint16_t vlan() { return 33; };
    uint16_t vlanmask() { return 0xff; } ;
    int vendorport_size() { return 1; };
    uint32_t vendorport(int) { return 1; };
};

struct EsalAppsRegMessage {
    EsalL2Filter fltr;
    EsalL2Filter &filter() { return fltr; };
};

#endif
#endif
