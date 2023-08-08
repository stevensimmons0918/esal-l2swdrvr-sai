/*
 *  esalSaiDefs.h 
 *
 *  Copyright (C) 2022, Fujitsu Networks Communications, Inc.
 *
 *  Brief: Global Definitions associated with ESAL SAI interface. 
 *
 *  Author: Steve Simmons
 */

#ifndef ESAL_VENDOR_API_HEADERS_ESALSAIDEF_H_ 
#define ESAL_VENDOR_API_HEADERS_ESALSAIDEF_H_

#include <stdint.h> 
#include <string>
#include <string.h>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sys/stat.h>
#include "esalSaiUtils.h"

#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif

#ifdef LARCH_ENVIRON
#include "esal_vendor_api/esal_vendor_api.h"
#endif

#ifdef UTS
#include "esal-vendor-api/headers/esalUnitTestDefs.h"
extern "C" {
extern bool useSaiFlag;
}
#else
extern "C" {
#include "sai/sai.h"
}
#endif

#ifdef LARCH_ENVIRON
#define SWERR(x)
#else
#include "lib/swerr.h"
#define SWERR(x)  Swerr::generate(x)
#endif

extern EsalSaiUtils saiUtils;

extern "C" {

extern sai_object_id_t esalSwitchId;
extern bool useSaiFlag;
extern bool esalPortTableFindId(sai_object_id_t portSai, uint16_t* portId);
extern bool esalPortTableFindSai(uint16_t portId, sai_object_id_t *portSai);
extern bool esalPortTableGetSaiByIdx(uint16_t idx, sai_object_id_t *portSai);
extern bool esalPortTableAddEntry(uint16_t portId, sai_object_id_t *portSai);
extern void esalPortTableState(sai_object_id_t portId, bool portState);
bool esalAddAclToPort(
    sai_object_id_t portSai, sai_object_id_t aclSai, bool ingr);
bool esalAddBroadcastPolicer(sai_object_id_t portSai,
                      sai_object_id_t policerSai);
bool esalAddMulticastPolicer(sai_object_id_t portSai,
                      sai_object_id_t policerSai);
bool get_policer_counter(uint16_t lPort, uint64_t *bcastGreenStats,
                         uint64_t *bcastRedStats, uint64_t *mcastGreenStats,
                         uint64_t *mcastRedStats);
bool clear_policer_counter(uint16_t lPort);
bool esalHandleSaiHostRxPacket(
    const void *buffer, sai_size_t bufferSz,
    uint32_t attrCnt, const sai_attribute_t *attrList);
extern bool esalFindBridgePortId(sai_object_id_t bridgePortSai,
                                 uint16_t *portId);
extern bool esalFindBridgePortSaiFromPortSai(
                sai_object_id_t portSai, sai_object_id_t *bridgePortSai);
extern bool esalFindBridgePortSaiFromPortId(
                uint16_t portId, sai_object_id_t *bridgePortSai);
extern bool esalPortTableSet(
                uint16_t tableIndex, sai_object_id_t portSai, uint16_t portId);
extern void esalPortSetStp(uint16_t portId, vendor_stp_state_t stpState);

extern bool esalBridgePortCreate(sai_object_id_t portSai,
                sai_object_id_t *bridgePortSai, uint16_t vlanId);
extern bool esalBridgePortRemove(sai_object_id_t portSai, uint16_t vlanId);
extern bool esalBridgePortListInit(uint32_t port_number);
extern void esalAlterForwardingTable(
                sai_fdb_event_notification_data_t *fdbNotify);

extern int16_t esalHostPortId;
extern char esalHostIfName[];

extern int esalCreateSaiHost(uint16_t portId, const char *name);
extern int esalRemoveSaiHost(void);
extern bool esalBridgeCreate(void);
extern bool esalBridgeRemove(void);
extern bool esalSetDefaultBridge(sai_object_id_t defaultBridgeSai);
extern const char *esalSaiError(sai_status_t rc);

extern bool esalStpCreate(sai_object_id_t *defStpId);
extern bool esalStpPortCreate(sai_object_id_t stpSai,
                    sai_object_id_t bridgePortSai, sai_object_id_t *stpPortSai);
extern void esalRestoreAdminDownPorts(void);

extern bool esalCreateBpduTrapAcl();
extern bool esalEnableBpduTrapOnPort(std::vector<sai_object_id_t>& portSaiList);
extern int esalVlanAddPortTagPushPop(uint16_t pPort, bool ingr, bool push);
extern std::map<std::string, std::string> esalProfileMap;
extern bool VendorWarmBootRestoreHandler();
extern bool VendorWarmBootSaveHandler();

#ifndef LARCH_ENVIRON
extern SFPLibInitialize_fp_t esalSFPLibInitialize;
extern SFPLibUninitialize_fp_t esalSFPLibUninitialize;
extern SFPLibraryRestart_fp_t esalSFPLibraryRestart;
extern SFPLibrarySupport_fp_t esalSFPLibrarySupport;
extern SFPRegisterL2ParamChangeCb_fp_t esalSFPRegisterL2ParamChangeCb;
extern SFPSetPort_fp_t esalSFPSetPort;
extern SFPGetPort_fp_t esalSFPGetPort;
extern SFPResetPort_fp_t esalSFPResetPort;
#endif

bool portCfgFlowControlInit();
bool perPortCfgFlowControlInit(uint16_t portNum);

typedef struct
{
    uint32_t index;
    uint32_t macAge;
    bool valid;
} macData;

struct aclTableAttributes {
    uint8_t field_out_port;
    uint8_t field_dst_ipv6;
    sai_s32_list_t* field_acl_range_type_ptr = nullptr;
    uint8_t field_tos;
    uint8_t field_ether_type;
    sai_acl_stage_t acl_stage;
    uint8_t field_acl_ip_type;
    sai_s32_list_t* acl_action_type_list_ptr = nullptr;
    uint8_t field_tcp_flags;
    uint8_t field_in_port;
    uint8_t field_dscp;
    uint8_t field_src_mac;
    uint8_t field_out_ports;
    uint8_t field_in_ports;
    uint8_t field_dst_ip;
    uint8_t field_l4_dst_port;
    sai_uint32_t size;
    uint8_t field_src_ipv6;
    uint8_t field_dst_mac;
    uint8_t field_tc;
    uint8_t field_icmpv6_type;
    uint8_t field_src_ip;
    uint8_t field_ip_protocol;
    uint8_t field_outer_vlan_id;
    uint8_t field_icmpv6_code;
    uint8_t field_ipv6_next_header;
    sai_s32_list_t* acl_bind_point_type_list_ptr = nullptr;
    uint8_t field_l4_src_port;
    uint8_t field_icmp_type;
    uint8_t field_icmp_code;
};

struct aclCounterAttributes {
    sai_object_id_t switch_id;
    sai_object_id_t table_id;
    sai_uint64_t packets;
    sai_uint64_t bytes;
    uint8_t enable_byte_count;
    uint8_t enable_packet_count;
};
struct aclEntryAttributes {
    sai_object_id_t switch_id;
    sai_acl_field_data_t field_out_ports;
    sai_acl_action_data_t action_egress_samplepacket_enable;
    sai_acl_action_data_t action_mirror_ingress;
    sai_acl_action_data_t action_set_policer;
    uint8_t admin_state;
    sai_acl_field_data_t field_l4_src_port;
    sai_acl_field_data_t field_ip_protocol;
    sai_acl_field_data_t field_l4_dst_port;
    sai_acl_field_data_t field_dscp;
    sai_acl_field_data_t field_ipv6_next_header;
    sai_acl_action_data_t action_mirror_egress;
    sai_uint32_t priority;
    sai_acl_field_data_t field_dst_mac;
    sai_acl_field_data_t field_in_port;
    sai_acl_field_data_t field_acl_ip_type;
    sai_acl_field_data_t field_src_ip;
    sai_acl_field_data_t field_tcp_flags;
    sai_acl_field_data_t field_outer_vlan_id;
    sai_acl_field_data_t field_dst_ip;
    sai_acl_action_data_t action_counter;
    sai_acl_field_data_t field_dst_ipv6;
    sai_acl_field_data_t field_tc;
    sai_acl_field_data_t field_tos;
    sai_object_id_t table_id;
    sai_acl_field_data_t field_acl_range_type;
    sai_acl_field_data_t field_icmp_type;
    sai_acl_field_data_t field_src_ipv6;
    sai_acl_field_data_t field_src_mac;
    sai_acl_field_data_t field_icmp_code;
    sai_acl_field_data_t field_ether_type;
    sai_acl_field_data_t field_out_port;
    sai_acl_action_data_t action_packet_action;
    sai_acl_action_data_t action_ingress_samplepacket_enable;
    sai_acl_field_data_t field_icmpv6_type;
    sai_acl_action_data_t action_set_outer_vlan_id;
    sai_acl_action_data_t action_redirect;
    sai_acl_field_data_t field_in_ports;
    sai_acl_field_data_t field_icmpv6_code;
};

extern sai_object_id_t packetFilterAclTableOid;
extern sai_object_id_t packetFilterIpV6AclTableOid;

bool esalCreateAclEntry(aclEntryAttributes attrAcl, sai_object_id_t& aclEntryOid);
bool esalRemoveAclEntry(sai_object_id_t acl_entry_id);
bool esalCreateAclTable(aclTableAttributes aclTableAttr, sai_object_id_t& aclTableId);

}

#define OID_VALUE_MASK 0x000000FFFFFFFFFFULL
#define GET_OID_VAL(oid) ((oid) & OID_VALUE_MASK)

const int ESAL_RC_OK = 0;
const int ESAL_RC_FAIL = 1;
const int ESAL_SAI_FAIL = -1;
const int ESAL_SFP_FAIL = -2;
const int ESAL_RESOURCE_EXH = -3;
const int ESAL_INVALID_PORT = -4;
const int ESAL_INVALID_VLAN = -5;

const int ESAL_UNITTEST_MAGIC_NUM = 155;

#endif  // ESAL_VENDOR_API_HEADERS_ESALSAIDEF_H_
