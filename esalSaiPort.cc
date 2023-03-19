/**
 * @file      esalSaiPort.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. Supports the SAIPORT object
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#include <cstddef>
#include <cstdint>
#ifdef HAVE_MRVL
#include "headers/esalCpssDefs.h"
#endif
#include "headers/esalSaiUtils.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cinttypes>
#include <mutex>
#include <vector>

#include <libconfig.h++>

#include "esal_vendor_api/esal_vendor_api.h"
#include "esal_warmboot_api/esal_warmboot_api.h"

#include "sai/sai.h"
#include "sai/saiport.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif

extern "C" {

// APPROACH TO SEMAPHORE: 
//   There are multiple threads for configuring the local port table,
//   as well as another thread for Packet Rx.  The following algorithm is 
//   used for protecting PORT ID <-map-> PORT SAI Object.  The following 
//   assumptions are built into the mutex design:
//   
//         - Both Port ID and SAI Object must be keys for look-up. 
//         - Port Tables only grow. 
//         - A typical port configuration is ~32 ports, maximum is 512. 
//         - Cannot use operating system primitives for Packet Rx/Tx.
//         - Can use operating system primitives for update
//
//  Therefore, there will be a "C" style arrays with entry containing two
//  fields: PORT ID and SAI Object. More importantly, there is a current
//  table size attribute.  All updates are made in the shadow area, above
//  the current table size.   Then, the current table size is incremented.
//  A sempahore protects the following sequence of code
//  
//  	o Instantiation of SAI Object.  
//  	o Writing to the C Array
//  	o Bumping the C Array Size
//
struct SaiPortEntry{
    uint16_t portId;
    sai_object_id_t portSai;
    bool isCopper = false;
    bool isSGMII = false;
    bool isChangeable = false;
    uint16_t lPort;
    bool autoneg;
    vendor_speed_t speed; 
    vendor_duplex_t duplex;
    bool adminState = false;
    bool operationState = false;
    int opStateDownCnt;
};

const int MAX_PORT_TABLE_SIZE = 512;
SaiPortEntry portTable[MAX_PORT_TABLE_SIZE];
#ifndef UTS
CPSS_PORT_MANAGER_SGMII_AUTO_NEGOTIATION_STC autoNegFlowControlCfg[MAX_PORT_TABLE_SIZE];
#endif
int portTableSize = 0;
std::mutex portTableMutex; 

void esalDumpPortTable(void) {

    static int cnt = 0; 
    if (cnt++ > 20) return;

    std::cout << "ESAL Port Table Size: " << portTableSize << "\n" << std::flush;
    for (auto i = 0; i < portTableSize; i++) {
        std::cout << "PortId: " << portTable[i].portId 
            << " PortSai: " << portTable[i].portSai 
            << " CU: " << portTable[i].isCopper 
            << " SGMII: " << portTable[i].isSGMII 
            << " CHNG: " << portTable[i].isChangeable 
            << " lPort: " << portTable[i].lPort 
            << " adm: " << portTable[i].adminState 
            << " op: " << portTable[i].operationState << "\n" << std::flush; 
    }
}

void processSerdesInit(uint16_t lPort);
void processRateLimitsInit(uint32_t lPort);
 
bool esalPortTableFindId(sai_object_id_t portSai, uint16_t* portId) {
    // Search array for match.
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portSai == portSai) {
            *portId = portTable[i].portId;
            return true;
        }
    }
    *portId = 0;
    return false; 
}

void esalPortSavePortAttr(
    uint16_t portId, uint16_t lPort, bool autoneg, vendor_speed_t speed, vendor_duplex_t duplex) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            portTable[i].lPort = lPort;
            portTable[i].autoneg = autoneg;
            portTable[i].speed = speed;
            portTable[i].duplex = duplex;
            return;
        }
    }
}

void esalPortTableSetCopper(uint16_t portId, bool isCopper) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            portTable[i].isCopper = isCopper;
            return;
        }
    }
    return;
}

void esalPortTableSetChangeable(uint16_t portId, bool isChange) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            portTable[i].isChangeable = isChange;
            return;
        }
    }
    return;
}

bool esalPortTableIsChangeable(uint16_t portId) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            return portTable[i].isChangeable;
        }
    }
    return false;
}

void esalPortTableSetIfMode(uint16_t portId) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            
            // No change if (isCopper and isSGMII) or (!isCopper and !SGMII)
            //  
#ifndef UTS
            bool autoneg = true;
            vendor_speed_t speed = VENDOR_SPEED_GIGABIT;
            vendor_duplex_t duplex = VENDOR_DUPLEX_FULL; 
            CPSS_PORT_INTERFACE_MODE_ENT ifMode; 
            CPSS_PORT_SPEED_ENT ifSpeed = CPSS_PORT_SPEED_1000_E;
            if (!portTable[i].isChangeable) {
                return; 
            } else if (portTable[i].isCopper && !portTable[i].isSGMII) {
                portTable[i].isSGMII = true;
                autoneg = portTable[i].autoneg;
                speed =  portTable[i].speed;
                duplex = portTable[i].duplex;
                ifMode = CPSS_PORT_INTERFACE_MODE_SGMII_E;
            } else if (!portTable[i].isCopper && portTable[i].isSGMII) {
                portTable[i].isSGMII = false;
                ifMode = CPSS_PORT_INTERFACE_MODE_1000BASE_X_E; 
            } else {
                return;
            }

            // Now, transitioning between SGMII and 1000Base_X with following steps:
            //  1. Delete the port from CPSS (required by SDK, or error returned
            //       when calling cpssDxChSamplePortManagerMandatoryParamsSet
            //  2. Call into cpssDxChSamplePortManagerMandatoryParamsSet to set ifmode:
            //     CPSS_PORT_INTERFACE_MODE_SGMII_E or 
            //     CPSS_PORT_INTERFACE_MODE_1000BASE_X_E
            //  3. Create the interface again for CPSS.
            //  4. Create the SAI port by calling into VendorSetPortRate.
            //  5. Set Flow Control Attributes.
            //  6. Set SerDes Attributes. 
            //
            CPSS_PORT_MANAGER_STC portEventStc;
            portEventStc.portEvent = CPSS_PORT_MANAGER_EVENT_DELETE_E;
            if (cpssDxChPortManagerEventSet(0, portId, &portEventStc)) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cpssDxChPortManagerEventSet fail1\n"));
            }
            if (cpssDxChSamplePortManagerMandatoryParamsSet(
                   0, portId, ifMode, ifSpeed, CPSS_PORT_FEC_MODE_DISABLED_E)){
                   SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                       SWERR_FILELINE, "cpssDxChSamplePortManagerMandatoryParamsSet fail2\n"));
            }
            portEventStc.portEvent = CPSS_PORT_MANAGER_EVENT_CREATE_E;
            if (cpssDxChPortManagerEventSet(0, portId, &portEventStc)) {
                 SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                     SWERR_FILELINE, "cpssDxChPortManagerEventSet fail4\n"));
            }
            (void) VendorSetPortRate(portTable[i].lPort, autoneg, speed, duplex);
            if (!perPortCfgFlowControlInit(portId)) {
                 SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                     SWERR_FILELINE, "perPortCfgFlowControlInit fail5\n"));
            }

#ifdef HAVE_MRVL
#ifndef LARCH_ENVIRON
            processSerdesInit(portTable[i].lPort);
#endif
#endif
            return;
#endif
        }
    }
    return;
}

void esalDetermineToRetrain(uint16_t portId,  bool linkstate) {
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {

            // Special case here is needed to force re-training of Copper SFP. 
            // That is, if CU SFP is in MAC LINK DOWN STATE for 2mins, retrain
            // by temporarily making 1000X, bouncing link.  Note, MAC_LINK_DOWN
            // is different than LINKDOWN.  Defect L900-2175.
            //
            if (linkstate || !portTable[i].isCopper) {
                portTable[i].opStateDownCnt = 0;
            } else {
                CPSS_PORT_MANAGER_STATUS_STC portStatus; 
                auto rc = cpssDxChPortManagerStatusGet(0, portId, &portStatus);
                if ((rc == GT_OK) && 
                    (portStatus.portState == CPSS_PORT_MANAGER_STATE_MAC_LINK_DOWN_E)) {

                    portTable[i].opStateDownCnt++;
                    if (portTable[i].opStateDownCnt > 120) {
                        portTable[i].opStateDownCnt = 0;
                        VendorResetPort(portTable[i].lPort) ;
                    }
                }
            }
            return;
        }
    } 
}

bool esalPortTableFindSai(uint16_t portId, sai_object_id_t *portSai) {
    // Search array for match.
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            *portSai = portTable[i].portSai;
            return true;
        }
    }
    *portSai = SAI_NULL_OBJECT_ID;
    return false; 
}

bool esalPortTableGetSaiByIdx(uint16_t idx, sai_object_id_t *portSai) {
    
    // Return port oid by idx if exist
    //
    if (portTable[idx].portSai != SAI_NULL_OBJECT_ID) {
        *portSai = portTable[idx].portSai;
        return true;
    }
    *portSai = SAI_NULL_OBJECT_ID;
    return false;
}

SaiPortEntry* esalPortTableGetEntryById(uint16_t portId) {
    for (int i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            return &portTable[i];
        }
    }
    return nullptr;
}

bool esalPortTableAddEntry(uint16_t portId, sai_object_id_t *portSai) {
    // Grab mutex.
    std::unique_lock<std::mutex> lock(portTableMutex);

    // Check for max exceeded. 
    if (portTableSize >= MAX_PORT_TABLE_SIZE) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "table full in esalPortTableAddEntry\n"));
        std::cout << "esalPortTableAddEntry: max table exceed: " << portId
                  << std::endl;
        return false;
    }

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalPortTableAddEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    if (portSai == SAI_NULL_OBJECT_ID) {
        // Instantiate a new port sai. Mandatory attributes 
        // Adding port speed now as 1G default, but assuming it will change.
        //
        std::vector<sai_attribute_t> attributes;
        sai_attribute_t attr;

        attr.id = SAI_PORT_ATTR_HW_LANE_LIST;
        std::vector<sai_uint32_t> hwLanes;
        hwLanes.push_back(portId); 

        attr.value.u32list.list = const_cast<sai_uint32_t*>(hwLanes.data());;
        attr.value.u32list.count = static_cast<sai_uint32_t>(hwLanes.size());
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_SPEED;
        attr.value.u32 = 1000;
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_ADMIN_STATE;
        attr.value.booldata = false;
        attributes.push_back(attr);

        // Create a port.
        retcode = saiPortApi->create_port(
            portSai, esalSwitchId, attributes.size(), attributes.data());
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "create_port Fail in " \
                                    "esalPortTableAddEntry\n"));
            std::cout << "create_port fail: " << esalSaiError(retcode)
                      << std::endl;
            return false;
        }
    }
#else
    *portSai = ESAL_UNITTEST_MAGIC_NUM;
#endif

    // Get id from oid
    uint16_t _portId = (uint16_t)GET_OID_VAL(*portSai);

    // Store first in shadow area, and then bump count. 
    portTable[portTableSize].portSai = *portSai;
    portTable[portTableSize].portId = _portId;
    portTableSize++;

    return true; 
}

bool esalAddAclToPort(sai_object_id_t portSai,
                      sai_object_id_t aclSai, bool ingr) {
    // Grab mutex.
    std::unique_lock<std::mutex> lock(portTableMutex);

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalPortTableAddEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    // Instantiate a new port sai. Mandatory attributes 
    // Adding port speed now as 1G default, but assuming it will change.
    sai_attribute_t attr;
    attr.id = (ingr ? SAI_PORT_ATTR_INGRESS_ACL : SAI_PORT_ATTR_EGRESS_ACL);
    attr.value.oid = aclSai;

    // Create a port.
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_port Fail in esalPortTableAddEntry\n"));
        std::cout << "set port fail: " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

#endif
    return true;
}

bool perPortCfgFlowControlInit(uint16_t portNum) {
#ifndef UTS
    uint8_t devNum = 0;
    CPSS_DXCH_PORT_AP_PARAMS_STC tmpStsParams; 
    GT_BOOL apEnable;

// Port configuration update
    if (autoNegFlowControlCfg[portNum].readyToUpdFlag == GT_TRUE) {
        if (cpssDxChPortInbandAutoNegEnableSet(devNum, portNum, 
                    autoNegFlowControlCfg[portNum].inbandEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortInbandAutoNegEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortInbandAutoNegEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortDuplexAutoNegEnableSet(devNum, portNum, 
                autoNegFlowControlCfg[portNum].duplexEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortDuplexAutoNegEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortDuplexAutoNegEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortSpeedAutoNegEnableSet(devNum, portNum, 
                autoNegFlowControlCfg[portNum].speedEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortSpeedAutoNegEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortSpeedAutoNegEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortInBandAutoNegBypassEnableSet(devNum, portNum, 
                autoNegFlowControlCfg[portNum].byPassEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortInBandAutoNegBypassEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortInBandAutoNegBypassEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortFlowControlEnableSet(devNum, portNum, 
                (CPSS_PORT_FLOW_CONTROL_ENT)autoNegFlowControlCfg[portNum].flowCtrlEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortFlowControlEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortFlowControlEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortFlowCntrlAutoNegEnableSet(devNum, portNum, 
                autoNegFlowControlCfg[portNum].flowCtrlEnable, 
                autoNegFlowControlCfg[portNum].flowCtrlPauseAdvertiseEnable) != GT_OK){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "cpssDxChPortFlowCntrlAutoNegEnableSet fail in perPortCfgFlowControlInit\n"));
                std::cout << "cpssDxChPortFlowCntrlAutoNegEnableSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
            return false;
        }
        if (cpssDxChPortApPortConfigGet(devNum, portNum, &apEnable, &tmpStsParams) == GT_OK){
            if (tmpStsParams.fcAsmDir != CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E){
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "fcAsmDir isn't equal to CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E\n"));
                std::cout << "fcAsmDir isn't equal to CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E on port num " << portNum << std::endl;

                tmpStsParams.fcAsmDir = CPSS_DXCH_PORT_AP_FLOW_CONTROL_SYMMETRIC_E;
                if (cpssDxChPortApPortConfigSet(devNum, portNum, apEnable, &tmpStsParams) != GT_OK) {
                    SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "cpssDxChPortApPortConfigSet fail in perPortCfgFlowControlInit\n"));
                    std::cout << "cpssDxChPortApPortConfigSet fail in perPortCfgFlowControlInit for port num " << portNum << std::endl;
                }  
            }
        }     
    }
#endif
    return true;
}

bool portCfgFlowControlInit() {

#ifndef UTS
// Database init
    for (auto i = 0; i < MAX_PORT_TABLE_SIZE; i++){
        autoNegFlowControlCfg[i].readyToUpdFlag = GT_FALSE;
    }
#endif

#ifdef LARCH_ENVIRON

// Cfg file parse
    FILE *cfg_file;
    char *startP, *endP, *line = NULL;
    size_t len = 0;
    ssize_t read;
    int8_t params[7], portNum;

    if (!(cfg_file = fopen("iniFiles/portCfgAutoNeg.ini","r"))){
        printf("portCfgAutoNegParser open port configuration faile fail");
        return false;
    }

    while ((read = getline(&line, &len, cfg_file)) != -1){
        if (strstr(line, "#")){
            continue;
        }
        startP = line;
        endP = startP + len;
        portNum = (int8_t)strtol(startP+3, &endP, 10);
        for (auto i = 0; i < 7; i++){
            params[i] = (int8_t)strtol(line+7+i*2, &endP, 10);
            startP += 2;
        }
        autoNegFlowControlCfg[portNum].inbandEnable = 
                                    params[0]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].duplexEnable = 
                                    params[1]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].speedEnable = 
                                    params[2]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].byPassEnable = 
                                    params[3]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].flowCtrlEnable = 
                                    params[4]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].flowCtrlPauseAdvertiseEnable = 
                                    params[5]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].flowCtrlAsmAdvertiseEnable = 
                                    params[6]?GT_TRUE:GT_FALSE;
        autoNegFlowControlCfg[portNum].readyToUpdFlag = GT_TRUE;
    }
    free(line);
    fclose(cfg_file);
#else
    std::vector<uint32_t> lPorts;
    if (saiUtils.GetLogicalPortList(0, &lPorts)) {
        for(auto &lPort : lPorts){
            uint32_t devId;
            uint32_t pPort;
            EsalSaiUtils::flowCtrlAttrs fc;
            if (saiUtils.GetFlowCtrlAttr(lPort, devId, pPort, fc)){
#ifndef UTS
                autoNegFlowControlCfg[pPort].inbandEnable = (GT_BOOL) fc.inbandEnable;
                autoNegFlowControlCfg[pPort].duplexEnable = (GT_BOOL) fc.duplexEnable;
                autoNegFlowControlCfg[pPort].speedEnable = (GT_BOOL) fc.speedEnable;
                autoNegFlowControlCfg[pPort].byPassEnable = (GT_BOOL) fc.byPassEnable;
                autoNegFlowControlCfg[pPort].flowCtrlEnable = (GT_BOOL) fc.flowCtrlEnable;
                autoNegFlowControlCfg[pPort].flowCtrlPauseAdvertiseEnable =
                    (GT_BOOL) fc.flowCtrlPauseAdvertiseEnable;
                autoNegFlowControlCfg[pPort].flowCtrlAsmAdvertiseEnable =
                    (GT_BOOL) fc.flowCtrlAsmAdvertiseEnable;
                autoNegFlowControlCfg[pPort].readyToUpdFlag = GT_TRUE;
#endif
            }
        }
    }
#endif
    return true;
}

void processSerdesInit(uint16_t lPort) {
    uint32_t dev;
    uint32_t pPort;
    EsalSaiUtils::serdesTx_t tx;
    EsalSaiUtils::serdesRx_t rx;

    if (saiUtils.GetSerdesInfo(lPort, dev, pPort, tx, rx)) {
        if (tx.has_vals) {
#ifndef UTS
            // We have TX serdes override values, so manage those
            CPSS_PORT_SERDES_TX_CONFIG_STC txConfig;
            memset(&txConfig, 0, sizeof(txConfig));
            txConfig.type = CPSS_PORT_SERDES_AVAGO_E;

            txConfig.txTune.avago.post = tx.post;
            txConfig.txTune.avago.pre = tx.pre;
            txConfig.txTune.avago.pre3 = tx.pre3;
            txConfig.txTune.avago.atten = tx.atten;
            txConfig.txTune.avago.pre2 = tx.pre2;
            GT_STATUS rc = cpssDxChPortSerdesManualTxConfigSet(dev,
                                                               pPort,
                                                               0,  // lane
                                                               &txConfig);
            if (rc != GT_OK) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "CPSS serdes TX in processSerdesInit\n"));
                std::cout << "cpss serdes tx fail: " << rc << std::endl;
            }
#endif
        }

        if (rx.has_vals) {
#ifndef UTS
            // We have RX serdes override values, so manage those
            CPSS_PORT_SERDES_RX_CONFIG_STC rxConfig;
            memset(&rxConfig, 0, sizeof(rxConfig));
            rxConfig.type = CPSS_PORT_SERDES_AVAGO_E;
            rxConfig.rxTune.avago.DC = rx.DC;
            rxConfig.rxTune.avago.LF = rx.LF;
            rxConfig.rxTune.avago.sqlch = rx.sqlch;
            rxConfig.rxTune.avago.HF = rx.HF;
            rxConfig.rxTune.avago.BW = rx.BW;
            
            GT_STATUS rc = cpssDxChPortSerdesManualRxConfigSet(dev,
                                                               pPort,
                                                               0,  // lane
                                                               &rxConfig);
            if (rc != GT_OK) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "CPSS serdes RX in processSerdesInit\n"));
                std::cout << "cpss serdes rx fail: " << rc << std::endl;
            }
#endif
        }
    }
    else {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "processSerdesInit lPort lookup fail\n"));
    }
}

bool esalAddBroadcastPolicer(sai_object_id_t portSai,
                      sai_object_id_t policerSai) {
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalAddBroadcastPolicer\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_BROADCAST_STORM_CONTROL_POLICER_ID;
    attr.value.oid = policerSai;

    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_port Fail in esalAddBroadcastPolicer\n"));
        std::cout << "set port fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

#endif
    return true;
}

bool esalAddMulticastPolicer(sai_object_id_t portSai,
                      sai_object_id_t policerSai) {
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalAddMulticastPolicer\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_FLOOD_STORM_CONTROL_POLICER_ID;
    attr.value.oid = policerSai;

    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_port Fail in esalAddMulticastPolicer\n"));
        std::cout << "set port fail: " << esalSaiError(retcode)
                  << std::endl;
        return false;
    }

#endif
    return true;
}

int VendorSetPortRate(uint16_t lPort, bool autoneg,
                      vendor_speed_t speed, vendor_duplex_t duplex) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    int rc  = ESAL_RC_OK;
    uint32_t pPort;
    uint32_t dev;
#ifndef UTS
    bool isCopper = false; 
#endif

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorSetPortRate failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }
#ifndef LARCH_ENVIRON
#ifndef UTS
    std::string hwid_value = esalProfileMap["hwId"];
    bool isSFP = false; 
#endif
    // First check to see if supported by SFP library.
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAutoNeg;
        val.SFPVal.AutoNeg = autoneg;
        values.push_back(val); 
        val.SFPAttr = SFPSpeed;
        val.SFPVal.LinkSpeed = speed;
        values.push_back(val); 
        val.SFPAttr = SFPDuplex;
        val.SFPVal.LinkDuplex = duplex;
        values.push_back(val); 
        if (!esalSFPSetPort) return ESAL_RC_FAIL;
        esalSFPSetPort(lPort, values.size(), values.data());
        val.SFPAttr = SFPCopper;
        val.SFPVal.Copper = false;
        values.clear();
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(lPort, values.size(), values.data());
#ifndef UTS
        isCopper = values[0].SFPVal.Copper;
        isSFP = saiUtils.GetChangeable(lPort);
#endif
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#else
    std::string hwid_value = "ALDRIN2EVAL";
#endif
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in VendorSetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL; 
    }


    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_PORT_ATTR_SPEED;

    switch (speed) {
        case VENDOR_SPEED_TEN:
            attr.value.u32 = 10;
            break;
        case VENDOR_SPEED_HUNDRED:
            attr.value.u32 = 100;
            break;
        case VENDOR_SPEED_GIGABIT:
            attr.value.u32 = 1000;
            break;
        case VENDOR_SPEED_TWO_AND_HALF_GIGABIT:
            attr.value.u32 = 2500;
            break;
        case VENDOR_SPEED_TEN_GIGABIT:
            attr.value.u32 = 10000;
            break;
        default:
            attr.value.u32 = 1000;
            break; 
    }


    // Eval does not need to set speed or duplex mode.  It can confused the reasl hardware.
    if (hwid_value.compare("ALDRIN2EVAL") != 0) {
        attributes.push_back(attr); 
    }
#ifdef NOT_SUPPORTED_BY_SAI
    if (hwid_value.compare("ALDRIN2EVAL") != 0) {
        attr.id = SAI_PORT_ATTR_FULL_DUPLEX_MODE;
        attr.value.booldata = (duplex == VENDOR_DUPLEX_FULL) ? true : false; 
        attributes.push_back(attr); 
    }
#else // NOT_SUPPORTED_BY_SAI
    if (esalHostPortId == pPort && 
        ((hwid_value.compare("ALDRIN2XLFL") == 0) || 
         (hwid_value.compare("ALDRIN2EB3") == 0))) {
        attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
        attr.value.booldata = true;
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_FEC_MODE;
        attr.value.s32 = SAI_PORT_FEC_MODE_FC;
        attributes.push_back(attr);
            esalPortTableSetChangeable(pPort, true);

    } else if (esalHostPortId == pPort && hwid_value.compare("ALDRIN2EVAL") == 0) {
        attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
        attr.value.booldata = false;
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_FEC_MODE;
        attr.value.s32 = SAI_PORT_FEC_MODE_FC;
        attributes.push_back(attr);

    } else if (esalHostPortId == pPort && hwid_value.compare("AC3XILA") == 0) {
        attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
        attr.value.booldata = true;
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_FEC_MODE;
        attr.value.s32 = SAI_PORT_FEC_MODE_FC;
        attributes.push_back(attr);
    }
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    esalPortSavePortAttr(portNum, lPort, autoneg, speed, duplex);
    esalPortTableSetCopper(portNum, isCopper);
#ifndef LARCH_ENVIRON
    esalPortTableSetChangeable(portNum, isSFP);
#endif
    int cpssDuplexMode;
    if (duplex == VENDOR_DUPLEX_HALF)
        cpssDuplexMode = CPSS_PORT_HALF_DUPLEX_E;
    else
        cpssDuplexMode = CPSS_PORT_FULL_DUPLEX_E;

    GT_BOOL cppsAutoneg;
    if (autoneg)
        cppsAutoneg = GT_TRUE;
    else
        cppsAutoneg = GT_FALSE;

    // Do not alter the interface in the case of the EVAL card.
    //
    if (hwid_value.compare("ALDRIN2EVAL") != 0) {
        if (speed == VENDOR_SPEED_TEN || speed == VENDOR_SPEED_HUNDRED || speed == VENDOR_SPEED_GIGABIT) {
            if (cpssDxChPortDuplexModeSet(devNum, portNum, cpssDuplexMode) != 0) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorSetPortRate fail in cpssDxChPortDuplexModeSet\n"));
                std::cout << "VendorSetPortRate fail, for pPort: " << pPort << "\n";
                return ESAL_RC_FAIL;
            }
            if (cpssDxChPortInbandAutoNegEnableSet(devNum, portNum, cppsAutoneg) != 0) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorSetPortRate fail in cpssDxChPortInbandAutoNegEnableSet\n"));
                std::cout << "VendorSetPortRate fail, for pPort: " << pPort << "\n";
                return ESAL_RC_FAIL;
            }
        }
    }
#endif
#endif
    // Set the port attributes
    for (auto &curAttr : attributes) {
        retcode = saiPortApi->set_port_attribute(portSai, &curAttr);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "set_port_attribute Fail " \
                                "in VendorSetPortRate\n"));
            std::cout << "set_port fail: " << esalSaiError(retcode)
                      << std::endl;
        }
    }

#endif

    return rc;
}

int VendorGetPortRate(uint16_t lPort, vendor_speed_t *speed) {
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
#endif
    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetPortRate failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPSpeed;
        val.SFPVal.LinkSpeed = *speed;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(lPort, values.size(), values.data());
        *speed = values[0].SFPVal.LinkSpeed;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai = 0;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetPortRate\n"));
        std::cout << "esalPortTableFindSai fail: pPort=" << pPort << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_SPEED;
    attributes.push_back(attr); 

    // Set the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    switch (attributes[0].value.u32) {
        case 10:
            *speed = VENDOR_SPEED_TEN;
            break;
        case 100:
            *speed = VENDOR_SPEED_HUNDRED;
            break;
        case 1000:
            *speed = VENDOR_SPEED_GIGABIT;
            break;
        case 2500:
            *speed = VENDOR_SPEED_TWO_AND_HALF_GIGABIT;
            break;
        case 10000:
            *speed = VENDOR_SPEED_TEN_GIGABIT;
            break;
        default:
            *speed = VENDOR_SPEED_UNKNOWN;
            std::cout << "Switch statement speed fail: " << attributes[0].value.u32
                      << std::endl; 
    }
#endif

    return rc;
}

int VendorGetPortDuplex(uint16_t lPort, vendor_duplex_t *duplex) {
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
#endif
    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetPortDuplex failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }
#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPDuplex;
        val.SFPVal.LinkDuplex = *duplex;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(lPort, values.size(), values.data());
        *duplex = values[0].SFPVal.LinkDuplex;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortDuplex\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai " \
                            "fail in VendorGetPortDuplex\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }
#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_FULL_DUPLEX_MODE;
    attributes.push_back(attr);

    // Set the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute " \
                            "fail in VendorGetPortDuplex\n"));
        std::cout << "get_port_attr fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    *duplex = (attributes[0].value.booldata) ? VENDOR_DUPLEX_FULL :
                        VENDOR_DUPLEX_HALF;
#else // NOT_SUPPORTED_BY_SAI
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    int cpssDuplexMode;
    if (cpssDxChPortDuplexModeGet(devNum, portNum, &cpssDuplexMode) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortDuplex fail in cpssDxChPortDuplexModeGet\n"));
        std::cout << "VendorGetPortDuplex fail, for pPort: " << pPort << "\n";
        return ESAL_RC_FAIL;
    }
    *duplex = (cpssDuplexMode == CPSS_PORT_HALF_DUPLEX_E) ? VENDOR_DUPLEX_HALF : VENDOR_DUPLEX_FULL;
#endif
#endif
#endif

    return rc;
}

int VendorGetPortAutoNeg(uint16_t lPort, bool *aneg) {

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
#endif

    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetPortAutoNeg failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAutoNeg;
        val.SFPVal.AutoNeg = *aneg;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL; 
        esalSFPGetPort(lPort, values.size(), values.data());
        *aneg = values[0].SFPVal.AutoNeg;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortAutoNeg\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetPortAutoNeg\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL; 
    }
#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
    attributes.push_back(attr); 

    // Set the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail " \
                            "in VendorGetPortAutoNeg\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }

    *aneg = attributes[0].value.booldata;
#else
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    GT_BOOL cpssAutoneg;
    if (cpssDxChPortInbandAutoNegEnableGet(devNum, portNum, &cpssAutoneg) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAutoNeg fail in cpssDxChPortInbandAutoNegEnableGet\n"));
        std::cout << "VendorGetPortAutoNeg fail, for pPort: " << pPort << "\n";
        return ESAL_RC_FAIL;
    }
    *aneg = (cpssAutoneg) ? true : false;
#endif
#endif


#endif

    SaiPortEntry* portEntry = esalPortTableGetEntryById(pPort);
    if (portEntry != nullptr) {
        portEntry->adminState = true;
    }

    return rc;
}

int VendorGetPortLinkState(uint16_t lPort, bool *ls) {
#ifdef DEBUG
     std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
#endif

    // Hack to hardcode link state to UP on eval card
    // To be removed once SFP Manager is suppported
    if ((saiUtils.GetUnitCode() == "feed") ||
        (saiUtils.GetUnitCode() == "FEED")) {
        if (ls != nullptr) {
            *ls = true;
            return ESAL_RC_OK;
        }
    }

    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetPortLinkState failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    if (esalPortTableIsChangeable(pPort)) {
        // Changeable need to check first to see if hardware has changed.
        // then get link state from L2SW. 
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPCopper;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL; 
        esalSFPGetPort(lPort, values.size(), values.data());
        esalPortTableSetCopper(pPort, values[0].SFPVal.Copper);
        esalPortTableSetIfMode(pPort);
    } else if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        // Check to see if supported by SFP library.
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPLinkStatus;
        val.SFPVal.LinkStatus = *ls;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL; 
        esalSFPGetPort(lPort, values.size(), values.data());
        *ls = values[0].SFPVal.LinkStatus;
        return rc; 
    }
#endif    
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortLinkState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetPortLinkState\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_OPER_STATUS;
    attributes.push_back(attr); 

    // Set the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail " \
                            "in VendorGetPortLinkState\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }

    *ls = (attributes[0].value.u32 == SAI_PORT_OPER_STATUS_UP) ? true : false;
    esalDetermineToRetrain(pPort, *ls);
#endif

    return rc;
}

int VendorEnablePort(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }


    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorEnablePort failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorEnablePort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorEnablePort\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    attr.value.booldata = true; 
 
    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorEnablePort\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }
   
    esalPortTableSetIfMode(pPort);

    if (!perPortCfgFlowControlInit(pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "perPortCfgFlowControlInit in VendorEnablePort fail\n"));
        return ESAL_RC_FAIL;
    }

#ifdef HAVE_MRVL
#ifndef LARCH_ENVIRON
    processSerdesInit(lPort);
    processRateLimitsInit(lPort);
#endif
#endif

#endif

    SaiPortEntry* portEntry = esalPortTableGetEntryById(pPort);
    if (portEntry != nullptr) {
        portEntry->adminState = true;
    }

    return rc;
}

int VendorDisablePort(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorEnablePort failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorDisablePort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        if (!esalPortTableAddEntry(pPort, &portSai)){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableFindSai fail " \
                                "in VendorDisablePort\n"));
            std::cout << "VendorAddMemberPort fail pPort:" << pPort << std::endl;
            return ESAL_RC_FAIL;
        }
    }

    // Add attributes. 
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    attr.value.booldata = false; 
 
    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorDisablePort\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }
#endif

    SaiPortEntry* portEntry = esalPortTableGetEntryById(pPort);
    if (portEntry != nullptr) {
        portEntry->adminState = false;
    }

    return rc;
}

int VendorSetFrameMax(uint16_t lPort, uint16_t size) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorSetFrameMax failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetFrameMax\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port. Create it if did not exist. 
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        if (!esalPortTableAddEntry(pPort, &portSai)){
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableAddEntry fail " \
                                "in VendorSetFrameMax\n"));
            std::cout << "VendorAddMemberPort fail pPort:" << pPort
                      << std::endl;
            return ESAL_RC_FAIL;
        }
    }

    // Add attributes. 
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_MTU;
    attr.value.u32 = size; 
 
    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail " \
                            "in VendorSetFrameMax\n"));
        std::cout << "sai_port_attribute fail:" << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

#endif

    return ESAL_RC_OK;
}

int VendorGetFrameMax(uint16_t lPort, uint16_t *size) {
    int rc  = ESAL_RC_OK;
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
#endif
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetFrameMax failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get api interface. 
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetFrameMax\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetFrameMax\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_MTU;
    attributes.push_back(attr); 

    // Set the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail " \
                            "in VendorGetFrameMax\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }

    *size = attributes[0].value.u32;
#endif
    return rc;
}

int VendorSetPortAdvertAbility(uint16_t lPort, uint16_t cap) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorSetPortAdvertAbility failed " \
                              "to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    // Set Port Advertising Capability
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAdvertise;
        val.SFPVal.AdvertAbility = (vendor_port_advert_ability) cap;
        values.push_back(val); 
        if (!esalSFPSetPort) return ESAL_RC_FAIL; 
        esalSFPSetPort(lPort, values.size(), values.data());
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
#ifdef NOT_SUPPORTED_BY_SAI
    // FIXME:  The following attributes are not yet supported. 
    // Documented here ... http://rtx-swtl-jira.fnc.net.local/browse/LARCH-3
    //
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetFrameMax\n"));
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    int32_t speedarr[3];
    sai_s32_list_t speedlst;
    speedlst.list = speedarr;
    speedlst.count = 0;

    if (cap & VENDOR_PORT_ABIL_1000MB_FD) {
        speedlst.list[speedlst.count++] = 1000;
    }

    if (cap & VENDOR_PORT_ABIL_100MB_FD) {
        speedlst.list[speedlst.count++] = 100;
    }

    if (cap & VENDOR_PORT_ABIL_10MB_FD) {
        speedlst.list[speedlst.count++] = 10;
    }

    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADVERTISED_SPEED;
    attr.value.s32list = speedlst;

    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }

    int32_t duplexarr[3];
    sai_s32_list_t duplexlst;
    duplexlst.list = duplexarr;
    duplexlst.count = 0;

    if (cap & VENDOR_PORT_ABIL_1000MB_HD) {
        duplexlst.list[duplexlst.count++] = 1000;
    }

    if (cap & VENDOR_PORT_ABIL_100MB_HD) {
        duplexlst.list[duplexlst.count++] = 100;
    }

    if (cap & VENDOR_PORT_ABIL_10MB_HD) {
        duplexlst.list[duplexlst.count++] = 10;
    }

    attr.id = SAI_PORT_ATTR_ADVERTISED_HALF_DUPLEX_SPEED;
    attr.value.s32list = duplexlst;

    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }
#else // NOT_SUPPORTED_BY_SAI
#if 0 //TODO
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    port_auto_neg_advertisment_t portAnAdvertismentPtr;
    if (cpssDxChPortAutoNegAdvertismentConfigGet(devNum, portNum, &portAnAdvertismentPtr) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAdvertAbility fail in cpssDxChPortAutoNegAdvertismentConfigGet\n"));
        std::cout << "VendorGetPortAdvertAbility fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
    *advert = 0;
    if (portAnAdvertismentPtr.duplex == CPSS_PORT_FULL_DUPLEX_E) {
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_FD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_FD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_FD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
    else { // CPSS_PORT_HALF_DUPLEX_E
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_HD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_HD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_HD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
#endif
#endif
#endif
    return ESAL_RC_OK;
}

int VendorGetPortAdvertAbility(uint16_t lPort, uint16_t *advert) {
    std::cout << __PRETTY_FUNCTION__ << " lPort:" << lPort  << std::endl;
#ifndef LARCH_ENVIRON
    int rc  = ESAL_RC_OK;
#endif
    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorGetFrameMax failed to get pPort\n"));
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(lPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAdvertise;
        val.SFPVal.AdvertAbility = *advert;
        values.push_back(val); 
        if (esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(lPort, values.size(), values.data());
        *advert = values[0].SFPVal.AdvertAbility;
        return rc;
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail " \
                            "in VendorGetPortAdvertAbility\n"));
        std::cout << "sai_api_query fail: " <<  esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorGetPortAdvertAbility\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    std::vector<sai_attribute_t> spdattributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADVERTISED_SPEED;
    spdattributes.push_back(attr);

    // Get the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                                spdattributes.size(), spdattributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail " \
                            "in VendorGetPortAdvertAbility\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }

    *advert = 0;
    auto spdlst = spdattributes[0].value.s32list;
    for(uint32_t spdi = 0; spdi < spdlst.count; spdi++) {
        switch (spdlst.list[spdi]) {
            case 10:
                *advert |= VENDOR_PORT_ABIL_10MB_FD;
                break;
            case 100:
                *advert |= VENDOR_PORT_ABIL_100MB_FD;
                break;
            case 1000:
                *advert |= VENDOR_PORT_ABIL_1000MB_FD;
                break;
            default: 
                std::cout << "Unknown full duplex speed: " << spdlst.list[spdi]
                          << std::endl;
        }
    }

    // Add attributes. 
    std::vector<sai_attribute_t> dupattributes;
    attr.id = SAI_PORT_ATTR_ADVERTISED_HALF_DUPLEX_SPEED;
    dupattributes.push_back(attr); 

    // Get the port attributes
    retcode = saiPortApi->get_port_attribute(portSai,
                            dupattributes.size(), dupattributes.data());

    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail " \
                            "in VendorGetPortAdvertAbility\n"));
        std::cout << "set_port fail:" << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL; 
    }

    auto duplst = spdattributes[0].value.s32list;
    for(uint32_t dupi = 0; dupi < duplst.count; dupi++) {
        switch (duplst.list[dupi]) {
            case 10:
                *advert |= VENDOR_PORT_ABIL_10MB_HD;
                break;
            case 100:
                *advert |= VENDOR_PORT_ABIL_100MB_HD;
                break;
            case 1000:
                *advert |= VENDOR_PORT_ABIL_1000MB_HD;
                break;
            default: 
                std::cout << "Unknown half speed: " << duplst.list[dupi]
                          << std::endl;
        }
    }
#else // NOT_SUPPORTED_BY_SAI
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC portAnAdvertismentPtr;
    if (cpssDxChPortAutoNegAdvertismentConfigGet(devNum, portNum, &portAnAdvertismentPtr) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAdvertAbility fail in cpssDxChPortAutoNegAdvertismentConfigGet\n"));
        std::cout << "VendorGetPortAdvertAbility fail, for pPort: " << pPort << "\n";
        return ESAL_RC_FAIL;
    }
    *advert = 0;
    if (portAnAdvertismentPtr.duplex == CPSS_PORT_FULL_DUPLEX_E) {
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_FD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_FD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_FD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
    else { // CPSS_PORT_HALF_DUPLEX_E
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_HD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_HD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_HD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
#endif
#endif
#endif

    return ESAL_RC_OK;
}

VendorL2ParamChangeCb_fp_t portStateChangeCb = 0;
void *portStateCbData = 0; 

// All SFPs will go through this callback. 
//
bool esalSfpCallback(
     void *cbId, uint16_t lPort, bool ls, bool aneg, vendor_speed_t spd, vendor_duplex_t dup)
{
    std::cout << "esalSfpCallback\n" << std::flush;
    uint32_t pPort;
    uint32_t dev;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalSfpCallback failed to get pPort\n"));
        return false;
    }

    if (esalPortTableIsChangeable(pPort)) {
        std::cout << "esalSfpCallback changeable\n";
        return true; 
    }

    if (!portStateChangeCb) return false;  
    return portStateChangeCb(cbId, lPort, ls, aneg, spd, dup);
}


int VendorRegisterL2ParamChangeCb(VendorL2ParamChangeCb_fp_t cb, void *cbId) {
    // Register global callback used for autoneg and link status. 
    std::unique_lock<std::mutex> lock(portTableMutex);
    portStateChangeCb = cb;
    portStateCbData = cbId;
#ifndef LARCH_ENVIRON
    // Hack to hardcode link state to UP on eval card
    // To be removed once SFP Manager is suppported
    if ((saiUtils.GetUnitCode() == "feed") ||
        (saiUtils.GetUnitCode() == "FEED")) {
        std::cout << __PRETTY_FUNCTION__ << " Publishing linsktate UP for eval"
                  << std::endl;
        if (cb == nullptr) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "SFPRegisterL2ParamChangeCb cb nullptr"));
            return ESAL_RC_FAIL;
        } else {
            std::vector<uint32_t> lPorts;
            if (saiUtils.GetLogicalPortList(0, &lPorts)) {
                for (uint32_t i = 0; i < lPorts.size(); i++) {
                    std::cout << __PRETTY_FUNCTION__ << " Publishing linsktate"
                              << " UP for lPort=" << i
                              << std::endl;
                    cb(cbId, i, true, true,
                       vendor_speed_t::VENDOR_SPEED_UNKNOWN,
                       vendor_duplex_t::VENDOR_DUPLEX_UNKNOWN);
                }
            }
            return ESAL_RC_OK;
        }
    }

    if (!esalSFPRegisterL2ParamChangeCb ||
         esalSFPRegisterL2ParamChangeCb(esalSfpCallback, cbId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "SFPRegisterL2ParamChangeCb fail " \
                            "in VendorRegisterL2ParamChangeCb\n"));
        std::cout << "VendorRegisterL2ParamChangeCb fail\n";
        return ESAL_RC_FAIL;
    }
#endif
    return ESAL_RC_OK;
}

void esalPortTableState(sai_object_id_t portSai, bool portState){
    // Verify if port exists within our provisioning. 
    uint16_t pPort;
    uint32_t lPort;

    if (!esalPortTableFindId(portSai, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE,
                    "esalPortTableFindId fail in esalPortTableState\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return;
    }

    if (!saiUtils.GetLogicalPort(0, pPort, &lPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE,
                    "esalPortTableState failed to get lPort\n"));
        std::cout << "esalPortTableFindSai GetLogicalPort fail pPort: "
                  << pPort << std::endl;
        return;
    }

    std::cout << "esalPortTableState : " << pPort << ":" << portState << "\n" << std::flush;



#ifndef LARCH_ENVIRON
    // Check to see if the SFP supported by SFP Library. If so, just call
    // set link state, and let SFP library handle callback. 
    if (esalSFPLibrarySupport && 
        esalSFPLibrarySupport(lPort) && 
        !esalPortTableIsChangeable(pPort)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPLinkStatus;
        val.SFPVal.LinkStatus = portState;
        values.push_back(val);
        if (!esalSFPSetPort) return;
        esalSFPSetPort(lPort, values.size(), values.data());
        return;
    }
#endif
    if (!portStateChangeCb) return;

    vendor_speed_t speed;
    bool autoneg;
    vendor_duplex_t duplex;

    if (VendorGetPortRate(lPort, &speed) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortRate fail in esalPortTableState\n"));
        std::cout << "VendorGetPortRate fail lPort: " << lPort << std::endl;
    }

    if (VendorGetPortAutoNeg(lPort, &autoneg) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortAutoNeg fail " \
                            "in esalPortTableState\n"));
        std::cout << "VendorGetPortAutoNeg fail lPort: " << lPort << std::endl;
    }

    if (VendorGetPortDuplex(lPort, &duplex) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortDuplex fail " \
                            "in esalPortTableState\n"));
        std::cout << "VendorGetPortDuplex fail lPort: "
                  << lPort << std::endl;
    }
    portStateChangeCb(portStateCbData, lPort, portState,
                      autoneg, speed, duplex);
}

int VendorResetPort(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;

    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }

    VendorDisablePort(lPort);
    VendorEnablePort(lPort);
    return ESAL_RC_OK;
}

int VendorReadReg(uint16_t lPort, uint16_t reg, uint16_t *val) {
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
#ifdef HAVE_MRVL
    uint32_t pPort;
    uint32_t dev;
    int rc;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorReadReg failed to get pPort\n"));
        std::cout << "VendorReadReg GetPhysicalPortInfo fail, pPort: "
                  << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

    rc = cpssDxChPhyPortSmiRegisterRead(dev, pPort, reg, val);
    if (rc != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorReadReg fail " \
                            "in cpssDxChPhyPortSmiRegisterRead\n"));
        std::cout << "VendorReadReg fail, for dev: " << dev << ", pPort: " << pPort << ", rc =" << rc << std::endl;
        return ESAL_RC_FAIL;
    }
#endif
#endif
    return ESAL_RC_OK;
}

int VendorWriteReg(uint16_t lPort, uint16_t reg, uint16_t val) {
    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }
#ifndef UTS
#ifdef HAVE_MRVL
    uint32_t pPort;
    uint32_t dev;
    int rc;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorWriteReg failed to get pPort\n"));
        std::cout << "VendorReadReg GetPhysicalPortInfo fail, pPort: "
                  << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

    rc = cpssDxChPhyPortSmiRegisterWrite(dev, pPort, reg, val);
    if (rc != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorWriteReg fail " \
                            "in cpssDxChPhyPortSmiRegisterWrite\n"));
        std::cout << "VendorWriteReg fail, for dev: " << dev << ", pPort: " << pPort << ", rc =" << rc << std::endl;
        return ESAL_RC_FAIL;
    }
#endif
#endif
    return ESAL_RC_OK;
}

int VendorDropTaggedPacketsOnIngress(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort:" << lPort << " " << std::endl;
    uint32_t pPort;
    uint32_t dev;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorDropTaggedPacketsOnIngress failed " \
                              "to get pPort\n"));
        std::cout << "VendorDropTaggedPacketsOnIngress GetPhysicalPortInfo "
                  << "fail, pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail " \
                            "in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_DROP_TAGGED;
    attr.value.booldata = true; 
 
    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail " \
                            "in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "set_port fail:" << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    return ESAL_RC_OK;
}

int VendorDropUntaggedPacketsOnIngress(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort:" << lPort << " " << std::endl;
    uint32_t pPort;
    uint32_t dev;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "VendorDropUntaggedPacketsOnIngress failed " \
                              "to get pPort\n"));
        std::cout << "VendorDropUntaggedPacketsOnIngress GetPhysicalPortInfo "
                  << "fail, pPort: " << pPort << std::endl;
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get port table api
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail " \
                            "in VendorDropUntaggedPacketsOnIngress\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail " \
                            "in VendorDropUntaggedPacketsOnIngress\n"));
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_DROP_UNTAGGED;
    attr.value.booldata = true; 
 
    // Set the port attributes
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail " \
                            "in VendorDropUntaggedPacketsOnIngress\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    return ESAL_RC_OK;
}

static bool restorePorts(SaiPortEntry* portTable, int portTableSize) {
    bool status = true;
    for (int i = 0; i < portTableSize; i++) {
        if (!esalPortTableAddEntry(portTable[i].portId, &portTable[i].portSai)) {
            status &= false;
            std::cout << "Error esalPortTableAddEntry " << portTable[i].portId << std::endl;
        }

        uint16_t pPort = portTable[i].portId;
        uint32_t lPort;
        if (!saiUtils.GetLogicalPort(0, pPort, &lPort)) {
                std::cout << "Error GetLogicalPort: " << pPort << std::endl;
                status &= false;
                continue;
        }

        if (VendorSetPortRate(lPort, portTable[i].autoneg, portTable[i].speed, portTable[i].duplex) != ESAL_RC_OK) {
            std::cout << "Error VendorSetPortRate " << lPort << std::endl;
            status &= false;
            continue;
        }

        if (portTable[i].adminState) {
            if (VendorEnablePort(lPort) != ESAL_RC_OK) {
                std::cout << "Error VendorEnablePort " << lPort << std::endl;
                status &= false;
                continue;
            }
        } else {
            if (VendorDisablePort(lPort) != ESAL_RC_OK) {
                std::cout << "Error VendorDisablePort " << lPort << std::endl;
                status &= false;
                continue;
            }
        }

        bool ls;
        if (VendorGetPortLinkState(lPort, &ls) != ESAL_RC_OK) {
            std::cout << "Error VendorGetPortLinkState: " << lPort << std::endl;
            status &= false;
            continue;
        }

        // Check if state 
        if (ls != portTable[i].operationState) {
            std::cout << "Operation state isn't same on port: " << lPort << std::endl;
            status &= false;
            continue;
        }
    }

    return status;
}

static bool serializePortTableConfig(SaiPortEntry *portTable, const int portTableSize,
                                                            const std::string &fileName) {
    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    libconfig::Setting &portTableSetting =
            root.add("portTable", libconfig::Setting::TypeList);

    for (int i = 0; i < portTableSize; i++) {
        uint32_t lPort;
        if (!saiUtils.GetLogicalPort(0, portTable[i].portId, &lPort)) {
            continue;
        }
        libconfig::Setting &portEntry =
                portTableSetting.add(libconfig::Setting::TypeGroup);
        portEntry.add("portId", libconfig::Setting::TypeInt) = portTable[i].portId;
        portEntry.add("portSai", libconfig::Setting::TypeInt64) = static_cast<int64_t>(portTable[i].portSai);
        portEntry.add("autoneg", libconfig::Setting::TypeBoolean) = portTable[i].autoneg;
        portEntry.add("speed", libconfig::Setting::TypeInt) = portTable[i].speed;
        portEntry.add("duplex", libconfig::Setting::TypeInt) = portTable[i].duplex;
        portEntry.add("adminState", libconfig::Setting::TypeBoolean) = portTable[i].adminState;
    }

    try {
        cfg.writeFile(fileName.c_str());
        return true;
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error writing to file: " << ex.what() << std::endl;
        return false;
    }
}

static bool deserializePortTableConfig(SaiPortEntry *portTable, int *portTableSize,
                                                                const std::string &fileName) {
    libconfig::Config cfg;
    try {
        cfg.readFile(fileName.c_str());
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error reading file: " << ex.what() << std::endl;
        return false;
    } catch (const libconfig::ParseException &ex) {
        std::cout << "Error parsing file: " << ex.what() << " at line "
                  << ex.getLine() << std::endl;
        return false;
    }

    libconfig::Setting &portTableSetting = cfg.lookup("portTable");
    if (!portTableSetting.isList()) {
        std::cout << "portTable is not a list" << std::endl;
        return false;
    }

    *portTableSize = 0;
    for (int i = 0; i < portTableSetting.getLength(); ++i) {
        libconfig::Setting &portEntry = portTableSetting[i];

        int portId, speed, duplex;
        long long portSai;
        bool autoneg, adminState;

        if (!(portEntry.lookupValue("portId", portId)   &&
              portEntry.lookupValue("portSai", portSai) &&
              portEntry.lookupValue("autoneg", autoneg) &&
              portEntry.lookupValue("speed", speed) &&
              portEntry.lookupValue("duplex", duplex) &&
              portEntry.lookupValue("adminState", adminState))) {
            return false;
        }

        if (*portTableSize >= MAX_PORT_TABLE_SIZE) {
            std::cout << "portTableSize >= MAX_PORT_TABLE_SIZE" << std::endl;
            return false;
        }

        portTable[*portTableSize].portId = static_cast<uint16_t>(portId);
        portTable[*portTableSize].portSai = static_cast<sai_object_id_t>(portSai);
        portTable[*portTableSize].autoneg = autoneg;
        portTable[*portTableSize].speed = static_cast<vendor_speed_t>(speed);
        portTable[*portTableSize].duplex = static_cast<vendor_duplex_t>(duplex);
        portTable[*portTableSize].adminState = adminState;
        (*portTableSize)++;
    }

    return true;
}

static void printPortEntry(const SaiPortEntry& portEntry) {
    std::cout << "Port ID: " << std::dec << portEntry.portId
        << ", OID: 0x" << std::setw(16) << std::setfill('0') << std::hex << portEntry.portSai
        << (portEntry.adminState ? "\tUP" : "\tDOWN")
        << std::endl
        << "autoneg: " << std::dec << portEntry.autoneg
        << ", speed: " << std::dec << portEntry.speed
        << ", duplex: " << std::dec << portEntry.duplex << std::endl;
}

bool portWarmBootSaveHandler() {
    bool status = true;

    std::unique_lock<std::mutex> lock(portTableMutex);
 
    // Saving a current operation state for ports
    //
    for (int i = 0; i < portTableSize; i++) {
        bool ls;
        uint16_t pPort;
        uint32_t lPort;

        pPort = portTable[i].portId;

        if (!saiUtils.GetLogicalPort(0, pPort, &lPort)) {
            continue;
        }
        if (VendorGetPortLinkState(lPort, &ls) != ESAL_RC_OK) {
            std::cout << "Error VendorGetPortLinkState: " << lPort << std::endl;
            status &= false;
            continue;
        }

        portTable[i].operationState = ls;
    }

    if (!serializePortTableConfig(portTable, portTableSize, BACKUP_FILE_PORT)) {
        std::cout << "Error serializePortTableConfig" << std::endl;
        status &= false;
    }

    return status;
}

bool portWarmBootRestoreHandler () {
    bool status = true;

    SaiPortEntry portTable[MAX_PORT_TABLE_SIZE];
    int portTableSize = 0;

    status = deserializePortTableConfig(portTable, &portTableSize, BACKUP_FILE_PORT);
    if (!status) {
        std::cout << "Error deserializing vlan map" << std::endl;
        return false;
    }

    if (!portTableSize) {
        std::cout << "Port table is empty!" << std::endl;
        return false;
    }

    std::cout << "Founded port configurations:" << std::endl;
    for (int i = 0; i < portTableSize; i++) {
        printPortEntry(portTable[i]);
    }

    std::cout << std::endl;
    std::cout << "Restore process:" << std::endl;
    status = restorePorts(portTable, portTableSize);
    if (!status) {
        std::cout << "Error restore ports" << std::endl;
        return false;
    }

    return true;
}

void portWarmBootCleanHandler() {
    std::unique_lock<std::mutex> lock(portTableMutex);
    portTableSize = 0;
}

}
