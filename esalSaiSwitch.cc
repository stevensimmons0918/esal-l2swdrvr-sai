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

#include "esal_vendor_api/esal_vendor_api.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#include "threadutils/dll_util.h"
#endif

#include "sai/sai.h"
#include "sai/saiswitch.h"
#include "sai/saihostif.h"

//Default STP ID 
sai_object_id_t defStpId = 0;

EsalSaiUtils saiUtils;

std::vector<sai_object_id_t> bpdu_port_list;

extern "C" {

#ifndef LARCH_ENVIRON
SFPLibInitialize_fp_t esalSFPLibInitialize;
SFPLibUninitialize_fp_t esalSFPLibUninitialize;
SFPLibraryRestart_fp_t esalSFPLibraryRestart;
SFPLibrarySupport_fp_t esalSFPLibrarySupport;
SFPRegisterL2ParamChangeCb_fp_t esalSFPRegisterL2ParamChangeCb;
SFPSetPort_fp_t esalSFPSetPort;
SFPGetPort_fp_t esalSFPGetPort;
#endif
#ifndef LARCH_ENVIRON
#ifndef UTS
static DllUtil *sfpDll = 0;
#endif
#endif
bool useSaiFlag = false;
static uint16_t esalMaxPort = 0; 
uint16_t esalHostPortId;
char esalHostIfName[SAI_HOSTIF_NAME_SIZE];
std::map<std::string, std::string> esalProfileMap;
bool WARM_RESTART = false;
#ifndef LARCH_ENVIRON
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
                std::cout << "esalHostIfListParser error: unknown port state" << "\n";
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
#if 0
        VendorWarmRestartRequest();
#endif
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

void onPacketEvent(sai_object_id_t sid,
                   sai_size_t bufferSize,
                   const void *buffer,
                   uint32_t attrCount,
                   const sai_attribute_t *attrList) {
    (void) esalHandleSaiHostRxPacket(buffer, bufferSize, attrCount, attrList); 
}

#endif

sai_object_id_t esalSwitchId = SAI_NULL_OBJECT_ID;

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

    const char *esal_warm_env = std::getenv("PSI_resetReason");
    if (esal_warm_env != NULL && !strcmp(esal_warm_env, "warm"))
    {
        WARM_RESTART = true;
    }
    else
    {
        WARM_RESTART = false;
    }
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
    std::string hwid_value = "ALDRIN2EVAL";;
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
    uint16_t        portId;
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

#endif

    if (!portCfgFlowControlInit()) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "portCfgFlowControlInit fail\n"));
        std::cout << "portCfgFlowControlInit fail \n";
        return ESAL_RC_FAIL;
    }

    return ESAL_RC_OK;
}

int DllDestroy(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

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
    switchStateUp = false; 

#ifndef LARCH_ENVIRON
     // Inform SFP about cold restart. 
     //
     if (esalSFPLibraryRestart) {
         if (!esalSFPLibraryRestart(false)) {
             std::cout << "esalSFPLibraryRestart failed\n";
         }
     } else {
         std::cout << "esalSFPLibraryRestart uninitialized\n";
     }
#endif

     // Soft reset the switch
     // To be removed when warm restart support is added
     GT_STATUS rc = cpssDxChHwPpSoftResetSkipParamSet(0,
                        CPSS_HW_PP_RESET_SKIP_TYPE_ALL_E, GT_FALSE);
     if (rc == GT_OK) {
         rc = cpssDxChHwPpSoftResetTrigger(0);

         if (rc != GT_OK) {
             std::cout << "Failed to trigger soft reset" << std::endl;
         }
     } else {
         std::cout << "cpssDxChHwPpSoftResetSkipParamSet failed" << std::endl;
     }

    // Query to get switch_api
    //  

#ifndef UTS
    sai_switch_api_t *saiSwitchApi; 
    sai_status_t retcode = sai_api_query(SAI_API_SWITCH, (void**)&saiSwitchApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query Fail in VendorWarmRestartRequest\n"));
        std::cout << "sai_api_query failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 
   
    // Set switch attribute
    // 
    sai_attribute_t attr;
    attr.id = SAI_SWITCH_ATTR_RESTART_WARM;
    attr.value.booldata = true;
    retcode =  saiSwitchApi->set_switch_attribute(esalSwitchId, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_switch_attribute Fail in VendorWarmRestartRequest\n"));
        std::cout << "set_switch_attribute failed: " << retcode << "\n"; 
        return ESAL_RC_FAIL;
    } 
#endif

    return ESAL_RC_OK;
}

int VendorGetTemp(char *temp) {
    uint8_t devNum = 0;
    int32_t tmp;
    GT_STATUS rc;
    rc = cpssDxChDiagDeviceTemperatureGet(devNum, &tmp);

    if (rc != GT_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cpssDxChDiagDeviceTemperatureGet failed\n"));
        std::cout << "cpss cpssDxChDiagDeviceTemperatureGet fail: "
                  << rc << std::endl;
        return ESAL_RC_FAIL;
    } else {
        std::string tmp_str = std::to_string(tmp);
        strcpy(temp, tmp_str.c_str());
    }
    return ESAL_RC_OK;
}

int VendorConfigurationComplete()
{
    CPSS_SYSTEM_RECOVERY_INFO_STC recovery_info;
    GT_STATUS rc;

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
             return ESAL_RC_FAIL;
        }
    }
    return ESAL_RC_OK;
}
}
