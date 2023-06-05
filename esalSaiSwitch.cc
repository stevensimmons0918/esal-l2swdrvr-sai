/**
 * @file      esalSaiSwitch.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. SAI SWITCH
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#include "headers/esalSaiUtils.h"
#include "saitypes.h"
#ifdef HAVE_MRVL
#include "headers/esalCpssDefs.h"
#endif
#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <cinttypes>
#include <map>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "esal_vendor_api/esal_vendor_api.h"
#include "esal_warmboot_api/esal_warmboot_api.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#include "threadutils/dll_util.h"
#endif

#include "sai/sai.h"
#include "sai/saiswitch.h"
#include "sai/saihostif.h"
#include <pthread.h> 
#include <libconfig.h++>
#include "headers/esalSaiDip.h"

//Default STP ID 
sai_object_id_t defStpId = 0;

EsalSaiUtils saiUtils;
EsalSaiDips dip;

std::vector<sai_object_id_t> bpdu_port_list;
bool WARM_RESTART;


extern "C" {

#ifndef LARCH_ENVIRON
SFPLibInitialize_fp_t esalSFPLibInitialize;
SFPLibUninitialize_fp_t esalSFPLibUninitialize;
SFPLibraryRestart_fp_t esalSFPLibraryRestart;
SFPLibrarySupport_fp_t esalSFPLibrarySupport;
SFPRegisterL2ParamChangeCb_fp_t esalSFPRegisterL2ParamChangeCb;
SFPSetPort_fp_t esalSFPSetPort;
SFPGetPort_fp_t esalSFPGetPort;
SFPResetPort_fp_t esalSFPResetPort;
#endif
#ifndef LARCH_ENVIRON
#ifndef UTS
static DllUtil *sfpDll = 0;
#endif
#endif
bool useSaiFlag = false;
static uint16_t esalMaxPort = 0; 
int16_t esalHostPortId = -1;
char esalHostIfName[SAI_HOSTIF_NAME_SIZE];
std::map<std::string, std::string> esalProfileMap;
#ifndef UTS
extern macData *macAddressData;
#endif
#ifndef LARCH_ENVIRON

bool isHostPortUp(void) {
    bool ls = true;  // assuming link up for failed cases

    if (esalHostPortId != -1) {
        uint32_t pPort = esalHostPortId;
        uint32_t lPort;
        uint32_t dev = 0;
        if(saiUtils.GetLogicalPort(pPort, dev, &lPort)) {
            bool tmpLs;
            if (VendorGetPortLinkState(lPort, &tmpLs) == 0) {
                ls = tmpLs;
            }
        }
    }
    return ls;
}

bool isHostIfRunning(void) {
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, esalHostIfName, sizeof(ifr.ifr_name));
    bool ifRunning = true; 
    int sock = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) < 0) {
       // Choose to allow failed ioctl to say running to prevent 
       // continuous reboots if esalHostIfName is not defined. 
       ifRunning = true; 
    } else if (!(ifr.ifr_flags & IFF_RUNNING)) { 
       // Comms Manager marks interface up.  Avoid race condition. 
       // if interface is not IFF_UP consider IFF_RUNNING until IFF_UP.
       ifRunning = true; 
    } else {
       ifRunning = (ifr.ifr_flags & IFF_RUNNING) ? true : false;
    }
    close(sock);
    return ifRunning;
}

bool esalHealthLeave = false; 
bool esalHealthMonEnable = true;
int esalHealthMonitorDelay = 30;
int esalHealthMonitorCycle = 5;

void* esalHealthMonitor(void*) {
#ifndef UTS
    int failRunningCnt = 0;
    int failSwitchCnt = 0; 
    sleep(esalHealthMonitorDelay); 

    std::cout << "starting esal health monitor" << std::endl;

    // Health monitor continues to check for both the stack interface being
    // present, as well as communication to the switch over PCI. 
    //
    while(true) {
        if (esalHealthMonEnable) {
            // Check the host interface in the stack to be RUNNING. 
            //
            if (isHostIfRunning()) {
                failRunningCnt = 0;
            } else {
                failRunningCnt++;
                std::cout << "ESAL Health Chk NOT RUNNING: "
                    << esalHostPortId << "\n" << std::flush; 
            }

            // Check the Enable Configured 
            //
            GT_BOOL enabled;
            if (!cpssDxChCfgDevEnableGet(0, &enabled)) {
                if (enabled) {
                   failSwitchCnt= 0; 
                } else {
                   failSwitchCnt++;
                   std::cout << "Esal Health Chk enabled:" << enabled << "\n" << std::flush;
                }
            } else { 
                std::cout << "cpssDxChCfgDevEnableGet FAIL\n" << std::flush; 
            }

            // Check if CPU port up
            //
            if (isHostPortUp()) {
                failRunningCnt = 0;
            } else {
                failRunningCnt++;
                std::cout << "ESAL Health Chk link NOT UP: "
                    << esalHostIfName << "\n" << std::flush; 
            }

            // Give yourself 20 failures in a row.  This avoids temporary 
            // instability.
            if ((failRunningCnt > 20) || (failSwitchCnt > 20)) {
                std::cout << "ESAL Health Check IFFRUNNING: " << failRunningCnt 
                          << " SwitchCnt: " << failSwitchCnt << "\n" << std::flush; 
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "ESAL Health Chk failure\n"));
                assert(0); 
            }
        }

        // dlldestrory will trigger us to leave loop. 
        // 
        if (esalHealthLeave) break; 

        // just sleep.
        // 
        sleep(esalHealthMonitorCycle); 

    }
    pthread_exit(NULL); 
#endif

    return 0;
}

pthread_t esalHealthTid;
void esalCreateHealthMonitor(void)  {
#ifndef UTS
    if (pthread_create(&esalHealthTid, NULL, esalHealthMonitor, NULL) ){
        std::cout << "ERROR esalCreateHealthMonitor fail\n";
    }
    (void) pthread_setname_np(esalHealthTid, "ESALHealthCheck"); 
#endif
}

void loadSFPLibrary(void) {

    // Instantiate DLL Object.
    //
    std::cout << "SFP Library: " << SFPLibraryName << "\n";
#ifndef UTS
    sfpDll = new DllUtil(SFPLibraryName);

    // Get pointers to the respective routines
    //
    esalSFPLibInitialize =
        reinterpret_cast<SFPLibInitialize_fp_t>(sfpDll->getDllFunc("SFPLibInitialize"));
    esalSFPLibUninitialize =
        reinterpret_cast<SFPLibUninitialize_fp_t>(sfpDll->getDllFunc("SFPLibUninitialize"));
    esalSFPLibraryRestart =
        reinterpret_cast<SFPLibraryRestart_fp_t>(sfpDll->getDllFunc("SFPLibraryRestart"));
    esalSFPLibrarySupport =
        reinterpret_cast<SFPLibrarySupport_fp_t>(sfpDll->getDllFunc("SFPLibrarySupport"));
    esalSFPRegisterL2ParamChangeCb =
        reinterpret_cast<SFPRegisterL2ParamChangeCb_fp_t>(sfpDll->getDllFunc("SFPRegisterL2ParamChangeCb"));
    esalSFPSetPort =
        reinterpret_cast<SFPSetPort_fp_t>(sfpDll->getDllFunc("SFPSetPort"));
    esalSFPGetPort =
        reinterpret_cast<SFPGetPort_fp_t>(sfpDll->getDllFunc("SFPGetPort"));
    esalSFPResetPort =
        reinterpret_cast<SFPResetPort_fp_t>(sfpDll->getDllFunc("SFPResetPort"));
#endif
    
    // Initialize the SFP library. 
    if (esalSFPLibInitialize) esalSFPLibInitialize();

    if (esalSFPSetPort) {
        // Following sets read/write callbacks to access CPSS SMI Read/Write
        // Registers.  This is needed support for PIU access for SFP
        // functionality.
        //
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPWordRead;
        val.SFPVal.ReadWord = (SFPReadWordFunc) VendorReadReg;
        values.push_back(val);
        val.SFPAttr = SFPWordWrite;
        val.SFPVal.WriteWord = (SFPWriteWordFunc) VendorWriteReg;
        values.push_back(val);
        esalSFPSetPort(0, values.size(), values.data());
    }

}

static void unloadSFPLibrary(void) {

    // Undo the SFP Library. 
    //
    if (esalSFPLibUninitialize) esalSFPLibUninitialize();
#ifndef UTS
    delete sfpDll;
    sfpDll = 0; 
#endif
    esalSFPLibInitialize = 0;
    esalSFPLibUninitialize = 0;
    esalSFPLibraryRestart = 0;
    esalSFPLibrarySupport = 0;
    esalSFPRegisterL2ParamChangeCb = 0;
    esalSFPSetPort = 0;
    esalSFPGetPort = 0;
    esalSFPResetPort = 0;
}
#endif
#ifndef UTS
static const char* profileGetValue(sai_switch_profile_id_t profileId, const char* variable){
    (void) profileId; 

    if (variable == NULL) {
        return NULL;
    }

    std::map<std::string, std::string>::const_iterator it = esalProfileMap.find(variable);
    if (it == esalProfileMap.end()) {
        return NULL;
    }

    return it->second.c_str();
}
#endif

static std::map<std::string, std::string>::iterator esalProfileIter = esalProfileMap.begin();
#ifndef UTS
static int profileGetNextValue(
    sai_switch_profile_id_t profileId, const char** variable,  const char** value) {
    (void) profileId;

    if (value == NULL) {
        esalProfileIter = esalProfileMap.begin();
        return 0;
    }

    if (variable == NULL) {
        return -1;
    }

    if (esalProfileIter == esalProfileMap.end()) {
        return -1;
    }

    *variable = esalProfileIter->first.c_str();
    *value = esalProfileIter->second.c_str();


    esalProfileIter++;

    return 0;
}
#endif

int handleProfileMap(const std::string& profileMapFile) {

    if (profileMapFile.size() == 0) {
        return ESAL_RC_FAIL;
    }

    std::ifstream profile(profileMapFile);

    if (!profile.is_open()) {
        return ESAL_RC_FAIL; 
    }

    std::string line;
    while(getline(profile, line)) {
        if (line.size() > 0 && (line[0] == '#' || line[0] == ';'))
            continue;

        size_t pos = line.find("=");

        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        std::cout << "ESAL SAI Profile: " << key << "=" << value << "\n";
        esalProfileMap[key] = value;
 
        if (key == "HostPortId") {
            esalHostPortId = std::stoi(value.c_str()); 
        } else if (key == "HostPortIfName") {
            memcpy(esalHostIfName, value.c_str(), SAI_HOSTIF_NAME_SIZE);
        }
    }
    return ESAL_RC_OK;
}

int esalHostIfListParser(std::string key , std::vector<sai_object_id_t>& out_vector) {
        char port_buf[3], value_buf[2];
        std::string inLine = esalProfileMap["hostIfListDisable"];
        size_t pos = inLine.find(":");
        sai_object_id_t portSaiTmp;
        
        while (inLine.size() > pos) {
            inLine.copy(port_buf, 3, pos-3);
            inLine.copy(value_buf, 1, pos+1);

            int port = atoi(port_buf);
            int value = atoi(value_buf);

            if (value == 0 && esalPortTableFindSai(port, &portSaiTmp)) {
                out_vector.erase(std::remove(out_vector.begin(), out_vector.end(), portSaiTmp), out_vector.end());
            } else {
                std::cout << "esalHostIfListParser error: unknown port state or non-existent port in sai.prifile.ini file. Port " << port << "\n";
            }

            
            pos += 6;
        }
    return ESAL_RC_OK;
}

#ifndef UTS

static const sai_service_method_table_t testServices = {
    profileGetValue,
    profileGetNextValue
};

#endif

static const char* EVAL_DRIVER_NAME =  "esal_l2_swdrvr_sai";

void VendorDbg(const char *args) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return;
    }
    std::cout << std::string(args) << std::endl;
}

esal_vendor_api_version_t VendorApiGetVersion() {
    return ((esal_vendor_api_version_t) {
            ESAL_VENDOR_API_VERSION_MAJOR,
            ESAL_VENDOR_API_VERSION_MAJOR});
}

bool switchStateUp = false; 

#ifndef UTS 
static void onSwitchStateChange(sai_object_id_t sid, sai_switch_oper_status_t switchOp)
{
    std::cout << "onSwitchStateChange: " << switchOp << " " << sid << "\n";
    if ((switchOp == SAI_SWITCH_OPER_STATUS_DOWN) && switchStateUp) {
        switchStateUp = false; 
    } else {
        switchStateUp = true;
    }
}

static void onFdbEvent(uint32_t count, sai_fdb_event_notification_data_t *data)
{
    (void) data; 
    for(uint32_t i = 0; i < count; i++) {
        (void) esalAlterForwardingTable(data+i);
    }
}


static void onPortStateChange(uint32_t count, sai_port_oper_status_notification_t *ntif)
{
    std::cout << "onPortStateChange: " << count << "\n";
    for(uint32_t i = 0; i < count; i++) {
        esalPortTableState(
            ntif[i].port_id, (ntif[i].port_state == SAI_PORT_OPER_STATUS_UP));
    }
}

#if 0
static int get_mac_addr(const char* interfaceName, sai_mac_t* mac) {
    struct ifreq ifrq = {0};

    if (mac == nullptr || interfaceName == nullptr) {
        return ESAL_RC_FAIL;
    }

    int fd = 0;
    if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0) {
        return ESAL_RC_FAIL;
    }

    strncpy(ifrq.ifr_name, interfaceName, IF_NAMESIZE);
    if (ioctl (fd, SIOCGIFHWADDR, &ifrq) < 0) {
        return ESAL_RC_FAIL;
    }
    memcpy(mac, ifrq.ifr_hwaddr.sa_data, 6);

    close (fd);

    return ESAL_RC_OK;
}
#endif

static int esalWarmRestartReNotifyFdb()
{
    CPSS_MAC_ENTRY_EXT_STC entry;
    GT_U8 cpssDevNum = 0;
    GT_HW_DEV_NUM associatedHwDevNum = 0;
    GT_BOOL valid;
    GT_BOOL skip = GT_FALSE;
    GT_BOOL aged[2] = {GT_FALSE, GT_FALSE};
    GT_STATUS rc;
    GT_U32 tblSize;
    sai_fdb_event_notification_data_t data;
    sai_attribute_t fdb_attribute[3];
    sai_packet_action_t saiAction = SAI_PACKET_ACTION_FORWARD;;

    rc = cpssDxChCfgTableNumEntriesGet(cpssDevNum, CPSS_DXCH_CFG_TABLE_FDB_E,
                                       &tblSize);
    if (rc != GT_OK)
    {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cpssDxChCfgTableNumEntriesGet failed\n"));
        std::cout << "cpssDxChCfgTableNumEntriesGet fail: "
                  << rc << std::endl;
        return ESAL_RC_FAIL;
    }

    for (GT_U32 entryIndex = 0; entryIndex < tblSize; entryIndex++)
    {
        rc = cpssDxChBrgFdbMacEntryRead(cpssDevNum, entryIndex, &valid, &skip,
                                        &aged[cpssDevNum], &associatedHwDevNum, &entry);
        if (rc != GT_OK)
        {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "cpssDxChBrgFdbMacEntryRead failed\n"));
                std::cout << "cpssDxChBrgFdbMacEntryRead fail: "
                          << rc << std::endl;
                return ESAL_RC_FAIL;
        }

        if (!valid)
        {
            continue;
        }

        // We should notify XPS layer also
        // Needs for address aging
#ifndef UTS
        if (!entry.isStatic)
        {
            macAddressData[entryIndex].valid = true;
            macAddressData[entryIndex].macAge = 0;
        }

#endif
        memset(&data, 0x0, sizeof(sai_fdb_event_notification_data_t));

        data.fdb_entry.switch_id = esalSwitchId;
        data.event_type = SAI_FDB_EVENT_LEARNED;

        data.fdb_entry.bv_id = ((uint64_t)SAI_OBJECT_TYPE_VLAN << 48) | entry.key.key.macVlan.vlanId;
        memcpy(data.fdb_entry.mac_address, entry.key.key.macVlan.macAddr.arEther, sizeof(sai_mac_t));

        data.attr_count = 3;

        //Fdb entry type
        fdb_attribute[0].id = SAI_FDB_ENTRY_ATTR_TYPE;
        fdb_attribute[0].value.s32 = (entry.isStatic == true) ?
                                     SAI_FDB_ENTRY_TYPE_STATIC : SAI_FDB_ENTRY_TYPE_DYNAMIC;

        fdb_attribute[1].id = SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID;
        if (!esalFindBridgePortSaiFromPortId(entry.dstInterface.devPort.portNum,
                                             &fdb_attribute[1].value.oid))
        {
            std::cout << "port_table_find_sai fail pPort:" << entry.dstInterface.devPort.portNum << std::endl;
            return ESAL_RC_FAIL;
        }

        //Packet action
        fdb_attribute[2].id = SAI_FDB_ENTRY_ATTR_PACKET_ACTION;
        switch (entry.daCommand)
        {
        case CPSS_MAC_TABLE_FRWRD_E:
                saiAction = SAI_PACKET_ACTION_FORWARD;
                break;

        case CPSS_MAC_TABLE_DROP_E:
                saiAction = SAI_PACKET_ACTION_DROP;
                break;

        case CPSS_MAC_TABLE_INTERV_E:
                saiAction = SAI_PACKET_ACTION_DROP;
                break;

        case CPSS_MAC_TABLE_CNTL_E:
                saiAction = SAI_PACKET_ACTION_TRAP;
                break;

        case CPSS_MAC_TABLE_MIRROR_TO_CPU_E:
                saiAction = SAI_PACKET_ACTION_COPY;
                break;

        case CPSS_MAC_TABLE_SOFT_DROP_E:
                break;

        default:
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "fdb entry DA command is unknown\n"));
                std::cout << "fdb entry DA command is unknown"
                          << rc << std::endl;
                return ESAL_RC_FAIL;
        }

        fdb_attribute[2].value.s32 = saiAction;

        data.attr = fdb_attribute;
 
        onFdbEvent(1, &data);
    }

    return ESAL_RC_OK;
}

void onPacketEvent(sai_object_id_t sid,
                   sai_size_t bufferSize,
                   const void *buffer,
                   uint32_t attrCount,
                   const sai_attribute_t *attrList) {
    (void) esalHandleSaiHostRxPacket(buffer, bufferSize, attrCount, attrList); 
}

#endif

sai_object_id_t esalSwitchId = SAI_NULL_OBJECT_ID;
int esalInitSwitch(std::vector<sai_attribute_t>& attributes, sai_switch_api_t *saiSwitchApi) {
#ifndef UTS
    sai_status_t retcode = ESAL_RC_OK;
    sai_attribute_t attr;

    retcode =  saiSwitchApi->create_switch(
        &esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_switch Fail in DllInit\n"));
        std::cout << "create failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 

    attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;

    retcode =  saiSwitchApi->get_switch_attribute(esalSwitchId, 1, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "get_switch_attribute Fail in DllInit\n"));
        std::cout << "get_switch_attribute failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 

    if (!esalSetDefaultBridge(attr.value.oid)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalSetDefaultBridge fail VendorAddPortsToVlan\n"));
            std::cout << "can't set default bridge object:" << "\n";
                return ESAL_RC_FAIL;

    }

    attr.id = SAI_SWITCH_ATTR_PORT_NUMBER;

    retcode =  saiSwitchApi->get_switch_attribute(esalSwitchId, 1, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "get_switch_attribute Fail in DllInit\n"));
        std::cout << "get_switch_attribute failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 
    uint32_t port_number = attr.value.u32;

    // Get port list //
    std::vector<sai_object_id_t> port_list;
    port_list.resize(port_number);

    attr.id = SAI_SWITCH_ATTR_PORT_LIST;
    attr.value.objlist.count = (uint32_t)port_list.size();
    attr.value.objlist.list = port_list.data();

    retcode =  saiSwitchApi->get_switch_attribute(esalSwitchId, 1, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
           SWERR_FILELINE, "get_switch_attribute Fail in DllInit\n"));
        std::cout << "get_switch_attribute failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 
 
    for (uint32_t i = 0; i < port_number; i++) {
        if (!esalPortTableAddEntry(i, &attr.value.objlist.list[i])) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableAddEntry fail in DllInit\n"));
            std::cout << "esalPortTableSet fail:" << "\n";
            return ESAL_RC_FAIL;
        }
        auto portId = (uint16_t)GET_OID_VAL(attr.value.objlist.list[i]);
        if (portId > esalMaxPort) {
            esalMaxPort = portId; 
        }
    }

    // Create default STP group
    if (!esalStpCreate(&defStpId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalStpCreate fail\n"));
        std::cout << "esalStpCreate fail:" << "\n";
        return ESAL_RC_FAIL;
    }

    // Get bridge ports from bridge
    if (!esalBridgePortListInit(port_number)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalBridgePortListInit fail\n"));
        std::cout << "esalBridgePortListInit fail:" << "\n";
        return ESAL_RC_FAIL;
    }
    
    // Create all host interfaces
    sai_object_id_t portSai;
    uint16_t portId;
    sai_object_id_t stpPortSai;
    sai_object_id_t bridgePortSai;
   
    for (uint32_t i = 0; i < port_number; i++) {

        if (!esalPortTableGetSaiByIdx(i, &portSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                 SWERR_FILELINE, "esalPortTableFindSai fail in DllInit\n"));
            std::cout << "esalPortTableFindSai fail:" << "\n";
            return ESAL_RC_FAIL;
        }

        portId = (uint16_t)GET_OID_VAL(portSai);

        if (!esalFindBridgePortSaiFromPortId(portId, &bridgePortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                 SWERR_FILELINE, "esalFindBridgePortSaiFromPortId fail\n"));
            std::cout << "can't find portid for bridgePortSai:" << bridgePortSai << "\n";
            return ESAL_RC_FAIL;
        }

        if (!esalStpPortCreate(defStpId, bridgePortSai, &stpPortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalStpPortCreate fail in DllInit\n"));
            std::cout << "esalStpPortCreate fail:" << "\n";
            return ESAL_RC_FAIL;
        }
    }

    if (!esalCreateBpduTrapAcl()) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalCreateBpduTrapAcl fail\n"));
        std::cout << "can't create bpdu trap acl \n";
        return ESAL_RC_FAIL;
    }

    for (uint ii = 0; ii < port_list.size(); ii++) {
        bpdu_port_list.push_back(port_list[ii]);
    }

    if (esalProfileMap.count("hostIfListDisable")) {
        esalHostIfListParser("hostIfListDisable", bpdu_port_list);
    }

    if (!esalEnableBpduTrapOnPort(bpdu_port_list)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalEnableBpduTrapOnPort fail\n"));
        std::cout << "can't enable bpdu trap acl \n";
        return ESAL_RC_FAIL;
    }

#else
    (void) attributes;
    (void) saiSwitchApi;
#endif // UTS

    if (!portCfgFlowControlInit()) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "portCfgFlowControlInit fail\n"));
        std::cout << "portCfgFlowControlInit fail \n";
        return ESAL_RC_FAIL;
    }

#ifndef LARCH_ENVIRON
    esalCreateHealthMonitor();
#endif

    return ESAL_RC_OK;
}


int DllInit(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    // load the sfp library.
    //
#if !defined(UTS) && !defined(LARCH_ENVIRON)
    loadSFPLibrary();
#endif

    // Verify that a config file is present first. 
    //
    std::string marvellScript(saiUtils.GetCfgPath("mvll.cfg"));
    auto fptr = fopen(marvellScript.c_str(), "r");
    if (fptr) {
        // Now, send the appDemo command if file exists. 
        //
        fclose(fptr);
        std::string cmdLine("/usr/bin/appDemo -daemon -config ");
        cmdLine.append(marvellScript);
        if (auto retcode = std::system(cmdLine.c_str())) {
            std::cout << "appdemo failed: " << retcode << "\n";
        }
        return ESAL_RC_OK;
    } else {
       std::cout << "Marvell cfg file not found: " << marvellScript << "\n";
       useSaiFlag = true;
    }

    // std::string fn(saiUtils.GetCfgPath("sai"));
    // handleProfileMap(fn);
    std::string profile_file(saiUtils.GetCfgPath("sai.profile.ini"));
    std::cout << "profile file: " << profile_file << "\n";

    if (handleProfileMap(profile_file) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "handleProfileMap Fail in DllInit\n"));
        std::cout << "Configuration file not found at " << profile_file << std::endl;
#ifndef LARCH_ENVIRON
        useSaiFlag = false;
        return ESAL_RC_FAIL;
#endif
    }

    if (!esalProfileMap.count("hwId")) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "hwId read Fail in DllInit\n"));
        std::cout << "Configuration file must contain at least hwId setting" << profile_file << std::endl;
#ifndef LARCH_ENVIRON
        useSaiFlag = false;
        return ESAL_RC_FAIL;
#endif
    }

#ifndef UTS

    // Initialize the SAI.
    //
    sai_api_initialize(0, &testServices);

    // Query to get switch_api
    //  
    sai_switch_api_t *saiSwitchApi; 
    sai_status_t retcode = sai_api_query(SAI_API_SWITCH, (void**)&saiSwitchApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in DllInit\n"));
        std::cout << "sai_api_query failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 

    // Determine which switch attributes to set. 
    // 
    std::vector<sai_attribute_t> attributes;

    sai_attribute_t attr;
    
    attr.id = SAI_SWITCH_ATTR_INIT_SWITCH;
    attr.value.booldata = true;
    attributes.push_back(attr); 

    attr.id = SAI_SWITCH_ATTR_SWITCH_STATE_CHANGE_NOTIFY;
    attr.value.ptr = reinterpret_cast<sai_pointer_t>(&onSwitchStateChange);
    attributes.push_back(attr); 

    attr.id = SAI_SWITCH_ATTR_FDB_EVENT_NOTIFY;
    attr.value.ptr = reinterpret_cast<sai_pointer_t>(&onFdbEvent);
    attributes.push_back(attr); 

    attr.id = SAI_SWITCH_ATTR_PORT_STATE_CHANGE_NOTIFY;
    attr.value.ptr = reinterpret_cast<sai_pointer_t>(&onPortStateChange);
    attributes.push_back(attr); 

    attr.id = SAI_SWITCH_ATTR_PACKET_EVENT_NOTIFY;
    attr.value.ptr = reinterpret_cast<sai_pointer_t>(&onPacketEvent);
    attributes.push_back(attr);

    attr.id = SAI_SWITCH_ATTR_SWITCH_PROFILE_ID;
    attr.value.u32 = 0;
    attributes.push_back(attr); 

    attr.id = SAI_SWITCH_ATTR_SWITCH_HARDWARE_INFO;
#ifndef LARCH_ENVIRON
    std::string hwid_value = esalProfileMap["hwId"];
#else
    std::string hwid_value = "ALDRIN2EVAL";
#endif
    attr.value.s8list.list = (sai_int8_t*)calloc(hwid_value.length() + 1, sizeof(sai_int8_t));
    std::copy(hwid_value.begin(), hwid_value.end(), attr.value.s8list.list);
    attributes.push_back(attr);

    attr.id = SAI_SWITCH_ATTR_FDB_AGING_TIME;
    attr.value.u32 = 180;
    attributes.push_back(attr); 

#if 0 // Currently FNC does not need this.
    // If we don't set this attribute then all interfaces have random MAC.
    // But it is not affect send packet functionality
    // Adding fake mac address to debug purposes
    // In normal situation this mac
    // will be derived from sai.profile
    // FIXME: http://rtx-swtl-jira.fnc.net.local/browse/LARCH-4
    // Value must be determine by reading lladdr for eth interface. 
    //
    attr.id = SAI_SWITCH_ATTR_SRC_MAC_ADDRESS;
    memset(&attr.value.mac, 0, sizeof(attr.value.mac));
    if (get_mac_addr("eth0", &attr.value.mac) == ESAL_RC_OK) {
        attributes.push_back(attr);
    }
#endif

    auto bkupFile = fopen(BACKUP_FOLDER, "r");
    if (bkupFile) {
        fclose(bkupFile);
        const char *esal_warm_env = std::getenv("PSI_resetReason");
        if (esal_warm_env) {
            std::string resetReason(esal_warm_env);
            std::transform(resetReason.begin(), resetReason.end(),
                           resetReason.begin(),
                           std::ptr_fun <int, int>(std::toupper));
            if (resetReason.compare("WARM") == 0) {
                WARM_RESTART = true;
            } else {
                WARM_RESTART = false;
            }
        }
    }

    // No need to support WARM RESTART on Eval.  Right now, it creates
    // packet loop/storm w/o call to cpssDxChHwPpSoftResetTrigger.
    //
    if (hwid_value == "ALDRIN2EVAL") {
        WARM_RESTART = false;
    }

#ifndef UTS
    if (esalProfileMap.count("suppressWarmRestart")) {
        std::string suppressRestart = esalProfileMap["suppressWarmRestart"];
        if ((suppressRestart == "Y") || (suppressRestart == "y")) {
            std::cout << "Suppressing warm restart\n";
            WARM_RESTART = false;
        }
    }

    std::cout << "WARM RESTART: " << WARM_RESTART << "\n" << std::flush;

    // Retrieve the provisioned values for both delay and cycle time
    // for health monitor check.
    //
    if (esalProfileMap.count("healthCheckDelay")) {
        std::string healthCheckDelay = esalProfileMap["healthCheckDelay"];
        esalHealthMonitorDelay = std::stoi(healthCheckDelay.c_str());
        std::cout << "Health Check Monitor Delay: " 
                  << esalHealthMonitorDelay << "\n" << std::flush;

    }

    if (esalProfileMap.count("healthCheckCycle")) {
        std::string healthCheckCycle = esalProfileMap["healthCheckCycle"];
        esalHealthMonitorCycle = std::stoi(healthCheckCycle.c_str());
        std::cout << "Health Check Monitor Cycle: " 
                  << esalHealthMonitorCycle << "\n" << std::flush;
    }
#endif

    // The point we need to jump to to re-initialize (make a hard reset) if "hot boot restore" fails.
    //
    retcode = esalInitSwitch(attributes, saiSwitchApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalInitSwitch Fail in DllInit\n"));
        std::cout << "esalInitSwitch failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 
#endif

    if (WARM_RESTART) {
#ifndef UTS
        if (!VendorWarmBootRestoreHandler()) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorWarmBootRestoreHandler fail\n"));
            std::cout << "VendorWarmBootRestoreHandler fail \n";
            WARM_RESTART = false;
            VendorWarmBootCleanHanlder();

            // Reinit switch (cold boot)
            retcode = esalInitSwitch(attributes, saiSwitchApi);
            if (retcode) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalInitSwitch Fail in DllInit\n"));
                std::cout << "esalInitSwitch failed: " << esalSaiError(retcode) << "\n"; 
                return ESAL_RC_FAIL;
            }
       }
#endif
    }

#ifndef UTS
    // Remove the backup folder.
    //
    std::string rmCmd("rm -rf ");
    rmCmd.append(BACKUP_FOLDER); 
    if (system(rmCmd.c_str())) {
        std::cout << "DllInit: fail rm cmd: " <<  BACKUP_FOLDER << "\n";
    }
#endif

    std::cout << "Dll Init after restore handler\n";

    return ESAL_RC_OK;
}

int DllDestroy(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    esalHealthLeave = true; 

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS

    // Query to get switch_api
    //  
    sai_switch_api_t *saiSwitchApi; 
    sai_status_t retcode = sai_api_query(SAI_API_SWITCH, (void**)&saiSwitchApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in DllDestroy\n"));
        std::cout << "sai_api_query failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 
   
    // Mark pre-shutdoown
    //   
    sai_attribute_t attr;
    attr.id = SAI_SWITCH_ATTR_PRE_SHUTDOWN;
    attr.value.booldata = true;
    retcode = saiSwitchApi->set_switch_attribute(esalSwitchId, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "set_switch_attribute Fail in DllDestroy\n"));
        std::cout << "set switch shutown: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    }
    
    // Remove all switch resoources. 
    //   
    retcode = saiSwitchApi->remove_switch(esalSwitchId);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "remove_switch Fail in DllDestroy\n"));
        std::cout << "remove switch fail: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    }
    sai_api_uninitialize();
    esalSwitchId = SAI_NULL_OBJECT_ID;


#endif 

    // Unload the SFP Library.
    //
#ifndef LARCH_ENVIRON
    unloadSFPLibrary();
#endif

    return ESAL_RC_OK;
}

void DllGetName(char *dllname) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::string nmstr(EVAL_DRIVER_NAME);
    memcpy(dllname, nmstr.c_str(), nmstr.length() + 1);
}

int VendorBoardInit(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    // WARNING: VendorBoardInit is different than DLL calls. 
    //    In this case, the returned value of "0" is SUCCESS, and all other
    //    returned values are FAILURE.
    //
    return ESAL_RC_OK;
}

uint16_t VendorGetMaxPorts(void) {
    auto rc = esalMaxPort+1;
    std::cout << __PRETTY_FUNCTION__ <<  rc << std::endl;
    return rc;
}

int VendorWarmRestartRequest(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

#ifndef UTS
    if (!VendorWarmBootSaveHandler()) {
        std::cout << "VendorWarmRestartRequest failed\n" << std::endl;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorWarmBootSaveHandler failed\n"));
    }
#endif

    return ESAL_RC_OK;
}

int VendorGetTemp(char *temp) {
#ifndef UTS
    uint8_t devNum = 0;
    int32_t tmp;
    GT_STATUS rc;
    rc = cpssDxChDiagDeviceTemperatureGet(devNum, &tmp);

    if (rc != GT_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cpssDxChDiagDeviceTemperatureGet failed\n"));
        return ESAL_RC_FAIL;
    } else {
        std::string tmp_str = std::to_string(tmp);
        strcpy(temp, tmp_str.c_str());
    }
#else
    (void) temp;
#endif
    return ESAL_RC_OK;
}

void VendorConfigBegin() {
    std::cout << "VendorConfigBegin begin\n";
}

void VendorConfigEnd()
{
#ifndef UTS
    CPSS_SYSTEM_RECOVERY_INFO_STC recovery_info;
    GT_STATUS rc;
    int status;
    std::cout << "VendorConfigEnd begin\n";

    if (WARM_RESTART)
    {
        memset(&recovery_info, 0, sizeof(CPSS_SYSTEM_RECOVERY_INFO_STC));
        recovery_info.systemRecoveryProcess = CPSS_SYSTEM_RECOVERY_PROCESS_HA_E;
        recovery_info.systemRecoveryState = CPSS_SYSTEM_RECOVERY_COMPLETION_STATE_E;
        recovery_info.systemRecoveryMode.haCpuMemoryAccessBlocked = GT_TRUE;

        rc = cpssSystemRecoveryStateSet(&recovery_info);

        if (rc != GT_OK)
        {
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                         SWERR_FILELINE, "cpssSystemRecoveryStateSet failed\n"));
             std::cout << "cpss cpssSystemRecoveryStateSet fail: "
                       << rc << std::endl;
             return;
        }

        status = esalWarmRestartReNotifyFdb();
        if (status != ESAL_RC_OK)
        {
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                         SWERR_FILELINE, "esalWarmRestartReNotifyFdb failed\n"));
             std::cout << "esalWarmRestartReNotifyFdb fail: "
                       << status << std::endl;
             return;
        }

        status = esalWarmRestartReNotifyFdb();
        if (status != ESAL_RC_OK)
        {
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                         SWERR_FILELINE, "esalWarmRestartReNotifyFdb failed\n"));
             std::cout << "esalWarmRestartReNotifyFdb fail: "
                       << status << std::endl;
             return;
        }
        WARM_RESTART = false; 
        esalRestoreAdminDownPorts();
        rc = cpssHalWarmResetComplete();
        if (rc != GT_OK)
        {
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                         SWERR_FILELINE, "cpssHalWarmResetComplete failed\n"));
             std::cout << "cpss cpssHalWarmResetComplete fail: "
                       << rc << std::endl;
             return;
        }
    }
#endif
    std::cout << "VendorConfigEnd end\n";
    return;
}

}
