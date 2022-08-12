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

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>
#include <cinttypes>
#include <map>
#include <string>
#include <vector>

#include "lib/swerr.h"
#include "esal-vendor-api/esal_vendor_api.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#include "threadutils/dll_util.h"
#endif


#ifndef UTS
#include "sai-vendor-api/sai.h"
#include "sai-vendor-api/saiswitch.h"
#include "sai-vendor-api/saihostif.h"
#endif

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
#ifndef SFP_RDY
#ifndef LARCH_ENVIRON
static DllUtil *sfpDll = 0;
#endif
#endif
uint16_t esalHostPortId;
char esalHostIfName[SAI_HOSTIF_NAME_SIZE];
static std::map<std::string, std::string> esalProfileMap;
#ifndef LARCH_ENVIRON
static void loadSFPLibrary(void) {

    // Instantiate DLL Object.
    //
    std::cout << "SFP Library: " << SFPLibraryName << "\n";
#ifndef SFP_RDY
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
    //
    if (esalSFPLibInitialize) esalSFPLibInitialize();
}

static void unloadSFPLibrary(void) {

    // Undo the SFP Library. 
    //
    if (esalSFPLibUninitialize) esalSFPLibUninitialize();
#ifndef SFP_RDY
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

// The two routimes getPlatformUnitCode and determineCfgFile are
// cloned and owned from the esalUtils.cc file in repo pllatform/esal-base.
// Ideally, it would have been better to call the same routine in 
// esal-base; however, that violates architectural flow.  That is, 
// easl-base calls into esalVendor.so, and never the other way. 
//
static std::string getPlatformUnitCode(void) {
    std::string unitCode;

    // Read command line. 
    std::ifstream cmdline("/proc/cmdline");
    if (cmdline.is_open()) {
        std::string line;
        while (getline(cmdline, line)) {
            if ((line.find("simulatedRole")) != std::string::npos) {
                unitCode = "02_00_00_00";
            } else if ((line.find("platformRole")) != std::string::npos) {
                unitCode = "01_00_00_00";
                break;
            }
        }
        cmdline.close();
    } else {
        std::cout << __FUNCTION__ << " Failed to open cmdline file\n";
    }

    // If not on command line, go to ENV variable. 
    //
    if (unitCode.empty()) {
        unitCode = "UNKNOWN"; 
        const char* buf = NULL;
        if ((buf = getenv("PSI_unitCode")) != NULL) {
            unitCode = buf;
        }
    }

    // keep the unitCode uppercase to avoid multiple comparisons
    //
    std::transform(unitCode.begin(), unitCode.end(), unitCode.begin(),
    std::ptr_fun<int, int>(std::toupper));
    return unitCode;
}

std::string determineCfgFile(const std::string &fname) {

    std::string basePath("/usr/local/fnc/esalbase"); 
    basePath += "/"; 
    basePath += getPlatformUnitCode(); 

    // Get FWDL from environment variable. 
    //
    std::string fwdlType("UNKNOWN");
    const char *buf = 0; 
    if ((buf = getenv("PSI_fwdlType")) != NULL) {
        fwdlType = buf; 
    }
    std::string fwdlPath = basePath + "/" + fwdlType;

    // Check for existence of this subdir
    //
    struct stat fs;
    if ((::stat(fwdlPath.c_str(), &fs)) == 0) {
        basePath = fwdlPath;  
    }
    std::string path; 
    path = basePath + "/" + fname + ".cfg";
    
    if (FILE *file = fopen(path.c_str(), "r")) {
        fclose(file);
    } else {
        path = basePath + "/" + fname + ".cfg";
        if (FILE *file1 = fopen(path.c_str(), "r")) {
            fclose(file1);
        } else {
            path = "";
        }
    }

    return path; 
}

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

static void handleProfileMap(const std::string& profileMapFile) {

    if (profileMapFile.size() == 0) {
        return;
    }

    std::ifstream profile(profileMapFile);

    if (!profile.is_open()) {
        return; 
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
        VendorWarmRestartRequest();
    } else {
        switchStateUp = true;
    }
}

static void onFdbEvent(uint32_t count, sai_fdb_event_notification_data_t *data)
{
    (void) data; 
    std::cout << "onFdbEvent: " << count << "\n";
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

static void onShutdownRequest(sai_object_id_t sid)//
{
    std::cout << "onShutdownRequest: " << sid << "\n";
}

void onPacketEvent(sai_object_id_t sid,
                   const void *buffer,
                   sai_size_t bufferSize,
                   uint32_t attrCount,
                   const sai_attribute_t *attrList) {
    std::cout << "onPacketEvent: " << sid << "\n";
    (void) esalHandleSaiHostRxPacket(buffer, bufferSize, attrCount, attrList); 
}

#endif

sai_object_id_t esalSwitchId = SAI_NULL_OBJECT_ID;

int DllInit(void) {
    int rc = 1;
    std::cout << __PRETTY_FUNCTION__ <<  std::endl;

    std::string fn(determineCfgFile("sai"));
    handleProfileMap(fn);

    // Unload the SFP Library.
    //
#ifndef LARCH_ENVIRON
    loadSFPLibrary();
#endif

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
        return 0;
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

    // attr.id = SAI_SWITCH_ATTR_SHUTDOWN_REQUEST_NOTIFY;
    // attr.value.ptr = reinterpret_cast<sai_pointer_t>(&onShutdownRequest);
    // attributes.push_back(attr); 

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
    attr.value.s8list.list = (sai_int8_t*)malloc(sizeof(sai_int8_t) *
                                                              (strlen("ALDRIN2XLFL")+1));
    memset(attr.value.s8list.list, 0, strlen("ALDRIN2XLFL") + 1);
    strcpy((char*)attr.value.s8list.list, "ALDRIN2XLFL");
     
    attributes.push_back(attr); 
    
    attr.id = SAI_SWITCH_ATTR_FDB_AGING_TIME;
    attr.value.u32 = 0;
    attributes.push_back(attr); 

    // attr.id = SAI_SWITCH_ATTR_TYPE;
    // attr.value.u32 = SAI_SWITCH_TYPE_NPU;
    // attributes.push_back(attr); 

//   These are mandatory in the following condition... SAI_SWITCH_TYPE_PHY
//     as well as SAI_SWITCH_ATTR_REGISTER_READ and SAI_SWITCH_ATTR_REGISTER_WRITE.
//    attr.id = SAI_SWITCH_ATTR_HARDWARE_ACCESS_BUS;
//    attr.value.u32 = SAI_SWITCH_HARDWARE_ACCESS_BUS_MDIO;
//    attributes.push_back(attr); 
//
//    attr.id = SAI_SWITCH_ATTR_PLATFROM_CONTEXT;
//    attr.value.u64 = 1;
//    attributes.push_back(attr); 

//  These are mandatory SAI_SWITCH_TYPE_VOQ
//    attr.id = SAI_SWITCH_ATTR_SWITCH_ID;
//    attr.value.u32 = 1;
//    attributes.push_back(attr); 

    // attr.id = SAI_SWITCH_ATTR_MAX_SYSTEM_CORES;
    // attr.value.u32 = 1;
    // attributes.push_back(attr); 


    retcode =  saiSwitchApi->create_switch(
        &esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_switch Fail in DllInit\n"));
        std::cout << "create failed: " << esalSaiError(retcode) << "\n"; 
        return 0;
    } 

    attr.id = SAI_SWITCH_ATTR_DEFAULT_1Q_BRIDGE_ID;
    
    retcode =  saiSwitchApi->get_switch_attribute(esalSwitchId, 1, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "get_switch_attribute Fail in DllInit\n"));
        std::cout << "get_switch_attribute failed: " << esalSaiError(retcode) << "\n"; 
        return 0;
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
        return 0;
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
        return 0;
    } 

    for (uint32_t i = 0; i < port_number; i++) {
        if (!esalPortTableSet(i, attr.value.objlist.list[i], i)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalPortTableSet fail VendorAddPortsToVlan\n"));
            std::cout << "esalPortTableSet fail:" << "\n";
                return ESAL_RC_FAIL;
            
        }
    }

    // Create all bridge ports
    sai_object_id_t bridgePortSai;
    sai_object_id_t portSai;
    for (uint32_t i = 0; i < port_number; i++) {
        
        if (!esalPortTableFindSai(i, &portSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalPortTableFindSai fail VendorAddPortsToVlan\n"));
            std::cout << "esalPortTableFindSai fail:" << "\n";
                return ESAL_RC_FAIL;
            
        }
                
        if (!esalBridgePortCreate(portSai, &bridgePortSai, 0)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalBridgePortCreate fail VendorAddPortsToVlan\n"));
            std::cout << "esalBridgePortCreate fail:" << "\n";
                return ESAL_RC_FAIL;
            
        }
    }

#ifndef LARCH_ENVIRON
    // Default Bridge already here after create_switch function.
    // Marvell sai plugin supports only one bridge
    // Create Bridge 
    //
    esalBridgeCreate(); 
    // Bridge already here
#endif
#else

    // Verify that a config file is present first. 
    //
    std::string marvellScript(determineCfgFile("mvll"));
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
    } else {
       std::cout << "Marvell cfg file not found: " << marvellScript << "\n";
    }

#endif 
    return rc;
}

int DllDestroy(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

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

    return 1;
}

void DllGetName(char *dllname) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::string nmstr(EVAL_DRIVER_NAME);
    memcpy(dllname, nmstr.c_str(), nmstr.length() + 1);
}

int VendorBoardInit(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    // WARNING: VendorBoardInit is different than DLL calls. 
    //    In this case, the returned value of "0" is SUCCESS, and all other
    //    returned values are FAILURE.
    //
    return ESAL_RC_OK;
}

uint16_t VendorGetMaxPorts(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    uint16_t rc = 0; 

#ifndef UTS
    // Query to get switch_api
    //  
    sai_switch_api_t *saiSwitchApi; 
    sai_status_t retcode = sai_api_query(SAI_API_SWITCH, (void**)&saiSwitchApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "set_switch_attribute Fail in VendorGetMaxPorts\n"));
        std::cout << "sai_api_query failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 

    // Set switch attribute
    // 
    sai_attribute_t attr;
    attr.id = SAI_SWITCH_ATTR_NUMBER_OF_FABRIC_PORTS;
    retcode =  saiSwitchApi->get_switch_attribute(esalSwitchId, 1, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "get_switch_attribute Fail in VendorGetMaxPorts\n"));
        std::cout << "get_switch_attribute failed: " << esalSaiError(retcode) << "\n"; 
        return ESAL_RC_FAIL;
    } 

    rc = attr.value.u16;
#else
    rc = 2000;
#endif 

   
    return rc;
}

int VendorWarmRestartRequest(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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


}


