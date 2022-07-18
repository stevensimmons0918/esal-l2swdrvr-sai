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
typedef uint32_t sai_object_id_t;
typedef uint32_t sai_status_t;
typedef uint32_t sai_size_t;
typedef uint32_t sai_switch_profile_id_t;
typedef unsigned char sai_mac_t[6];
const int SAI_HOSTIF_NAME_SIZE = 16;
const sai_object_id_t SAI_NULL_OBJECT_ID = 0;

union saiValue {
    uint32_t oid;
    int32_t  s32;
};

struct sai_attribute_t { 
    uint32_t id; 
    saiValue value;
};

struct sai_fdb_entry_t {
    unsigned char mac_address[6];
    uint32_t bv_id; 
};

struct sai_fdb_event_notification_data_t { 
    sai_fdb_entry_t fdb_entry;
    int event_type; 
    int attr_count;
    sai_attribute_t *attr;
};

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
};

struct EsalAppsRegMessage {
    EsalL2Filter fltr;
    EsalL2Filter &filter() { return fltr; };
};

const int SAI_FDB_EVENT_LEARNED = 1;
const int SAI_FDB_EVENT_AGED = 2;
const int SAI_FDB_EVENT_MOVE = 3;
const int SAI_FDB_EVENT_FLUSHED = 4;
const int SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID = 5;
const int SAI_FDB_FLUSH_ATTR_BV_ID = 6;
const int SAI_FDB_FLUSH_ATTR_BRIDGE_PORT_ID = 7;
const int SAI_VLAN_MEMBER_ATTR_VLAN_TAGGING_MODE = 1;
const int SAI_VLAN_TAGGING_MODE_TAGGED = 2; 
const int SAI_ACL_STAGE_INGRESS = 55;
const int SAI_ACL_STAGE_EGRESS = 56;



#endif
#endif
