/*
 *  esalSaiDefs.h 
 *
 *  Copyright (C) 2022, Fujitsu Networks Communications, Inc.
 *
 *  Brief: Global Definitions associated with ESAL SAI interface. 
 *
 *  Author: Steve Simmons
 */

#ifndef _ESAL_SAI_DEFS_H_ 
#define _ESAL_SAI_DEFS_H_
#include <stdint.h> 
#include <string>
#include <string.h>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sys/stat.h>

#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif

#ifdef UTS
#include "esal-vendor-api/headers/esalUnitTestDefs.h"
#else
extern "C" {
#include "sai/sai.h"
}
#endif

#ifdef LARCH_ENVIRON
#define SWERR(x)
#else
#define SWERR(x)  Swerr::generate(x)
#endif

extern "C" {

extern sai_object_id_t esalSwitchId;
extern bool esalPortTableFindId(sai_object_id_t portSai, uint16_t* portId);
extern bool esalPortTableFindSai(uint16_t portId, sai_object_id_t *portSai);
extern bool esalPortTableAddEntry(uint16_t portId, sai_object_id_t *portSai);
extern void esalPortTableState(sai_object_id_t portId, bool portState);
bool esalAddAclToPort(
    sai_object_id_t portSai, sai_object_id_t aclSai, bool ingr);
bool esalHandleSaiHostRxPacket(
    const void *buffer, sai_size_t bufferSz, uint32_t attrCnt, const sai_attribute_t *attrList);
extern bool esalFindBridgePortId(sai_object_id_t bridgePortSai, uint16_t *portId);
extern bool esalFindBridgePortSaiFromPortSai(sai_object_id_t portSai, sai_object_id_t *bridgePortSai);
extern bool esalFindBridgePortSaiFromPortId(uint16_t portId, sai_object_id_t *bridgePortSai);
extern bool esalPortTableSet(uint16_t tableIndex, sai_object_id_t portSai, uint16_t portId);
extern bool esalBridgePortCreate(sai_object_id_t portSai, sai_object_id_t *bridgePortSai, uint16_t vlanId);
extern bool esalBridgePortRemove(sai_object_id_t portSai, uint16_t vlanId);
extern void esalAlterForwardingTable(sai_fdb_event_notification_data_t *fdbNotify);


extern uint16_t esalHostPortId;
extern char esalHostIfName[]; 

extern int esalCreateSaiHost(uint16_t portId, const char *name);
extern int esalRemoveSaiHost(void);
extern bool esalBridgeCreate(void);
extern bool esalBridgeRemove(void);
extern bool esalSetDefaultBridge(sai_object_id_t defaultBridgeSai);
extern const char *esalSaiError(sai_status_t rc);
#ifndef LARCH_ENVIRON
extern SFPLibInitialize_fp_t esalSFPLibInitialize;
extern SFPLibUninitialize_fp_t esalSFPLibUninitialize;
extern SFPLibraryRestart_fp_t esalSFPLibraryRestart;
extern SFPLibrarySupport_fp_t esalSFPLibrarySupport;
extern SFPRegisterL2ParamChangeCb_fp_t esalSFPRegisterL2ParamChangeCb;
extern SFPSetPort_fp_t esalSFPSetPort;
extern SFPGetPort_fp_t esalSFPGetPort;
#endif

}



const int ESAL_RC_OK = 0;
const int ESAL_RC_FAIL = 1; 
const int ESAL_SAI_FAIL = -1;
const int ESAL_SFP_FAIL = -2;
const int ESAL_RESOURCE_EXH = -3;
const int ESAL_INVALID_PORT = -4;
const int ESAL_INVALID_VLAN = -5;

const int ESAL_UNITTEST_MAGIC_NUM = 155;

#endif
