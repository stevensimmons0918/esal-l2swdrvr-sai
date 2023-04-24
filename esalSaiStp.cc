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
#include "headers/esalSaiUtils.h"
#include <iostream>
#include <iomanip>
#include <cinttypes>
#include <esal_vendor_api/esal_vendor_api.h>
#include "esal_warmboot_api/esal_warmboot_api.h"

#include <libconfig.h++>

#include "sai/sai.h"
#include "sai/saistp.h"

extern "C" {

struct StpGroupMember{
    uint16_t portId;
    sai_object_id_t stpSai;
    sai_object_id_t bridgePortSai;
    sai_object_id_t stpPortSai;
    vendor_stp_state_t stpState;
};

static std::vector<StpGroupMember> stpPortTable;
static std::mutex stpTableMutex;

bool esalFindStpPortSaiFromPortId(sai_object_id_t portId,
                                  sai_object_id_t *stpPortSai) {
    // Iterate over the Stp Port.
    // 
    for (auto &stpGroupMember : stpPortTable) {
        if (stpGroupMember.portId == portId) {
            *stpPortSai = stpGroupMember.stpPortSai; 
            return true; 
        }
    }

    return false; 
}

StpGroupMember* esalFindStpMemberByPortId(uint16_t portId) {
    for (auto& member : stpPortTable) {
        if (member.portId == portId) {
            return &member;
        }
    }
    return nullptr;
}

int VendorSetPortStpState(uint16_t lPort, vendor_stp_state_t stpState) {
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    std::unique_lock<std::mutex> lock(stpTableMutex);

    uint32_t dev;
    uint32_t pPort;
    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    esalPortSetStp(pPort, stpState);

#ifndef UTS
    sai_stp_api_t *saiStpApi;
    sai_attribute_t attr;

    auto retcode = sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    attr.id = SAI_STP_PORT_ATTR_STATE;
    switch (stpState) {
    case VENDOR_STP_STATE_LEARN:
        attr.value.s32 = SAI_STP_PORT_STATE_LEARNING;
        break;

    case VENDOR_STP_STATE_FORWARD:
        attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
        break;

    case VENDOR_STP_STATE_BLOCK:
        attr.value.s32 = SAI_STP_PORT_STATE_BLOCKING;
        break;
    
    default:
        attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
        break;
    }
  
    sai_object_id_t stpPortSai;
    if (!esalFindStpPortSaiFromPortId(pPort, &stpPortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalFindStpPortSaiFromPortId fail " \
                                  "VendorSetPortStpState\n"));
            std::cout << "can't find stp port object for pPort:" << pPort << "\n";
                return ESAL_RC_FAIL;    
    }
    
    retcode = saiStpApi->set_stp_port_attribute(stpPortSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    StpGroupMember* mbr = esalFindStpMemberByPortId(pPort);
    if (mbr == nullptr) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalFindStpMemberByPortId fail in VendorSetPortStpState\n"));
        std::cout << "esalFindStpMemberByPortId fail: nullptr was returned" << "\n";
        return ESAL_RC_FAIL;
    }
    mbr->stpState = stpState;
    
    return ESAL_RC_OK;
}

int VendorGetPortStpState(uint16_t lPort, vendor_stp_state_t *stpState) {
    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    sai_status_t retcode;
    sai_stp_api_t *saiStpApi;
    
    retcode = sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    attr.id = SAI_STP_PORT_ATTR_STATE;
    std::vector<sai_attribute_t> attributes;
    attributes.push_back(attr); 

    sai_object_id_t stpPortSai;
    if (!esalFindStpPortSaiFromPortId(pPort, &stpPortSai)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                  SWERR_FILELINE, "esalFindStpPortSaiFromPortId fail " \
                                  "VendorGetPortStpState\n"));
            std::cout << "can't find stp port object for pPort:" << pPort
                      << "\n";
                return ESAL_RC_FAIL;    
    }

    retcode = saiStpApi->get_stp_port_attribute(stpPortSai,
                                attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortStpState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    switch (attributes[0].value.s32)
    {
    case SAI_STP_PORT_STATE_LEARNING:
        *stpState = VENDOR_STP_STATE_LEARN;
        break;

    case SAI_STP_PORT_STATE_FORWARDING:
        *stpState = VENDOR_STP_STATE_FORWARD;
        break;

    case SAI_STP_PORT_STATE_BLOCKING:
        *stpState = VENDOR_STP_STATE_BLOCK;
        break;
    
    default:
        *stpState = VENDOR_STP_STATE_UNKNOWN;
        break;
    }
#endif
    
    return ESAL_RC_OK;
}

bool esalStpCreate(sai_object_id_t *defStpId) {

#ifndef UTS
    // Get the STP API
    //
    sai_stp_api_t *saiStpApi;
    auto retcode =  sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalStpCreate\n"));
        std::cout << "sai_api_query fail" << esalSaiError(retcode) << "\n";
        return false;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;

    // Create the STP object.
    //
    retcode = saiStpApi->create_stp(defStpId, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "STP object creation fails in esalStpCreate\n"));
        std::cout << "create_stp fail" << esalSaiError(retcode) << "\n";
        return false;
    }
#endif
    return true; 
}

bool esalStpPortCreate(sai_object_id_t stpSai, sai_object_id_t bridgePortSai, sai_object_id_t *stpPortSai) {
    std::unique_lock<std::mutex> lock(stpTableMutex);
    
#ifndef UTS
    sai_stp_api_t *saiStpApi;
    auto retcode =  sai_api_query(SAI_API_STP, (void**) &saiStpApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalStpPortCreate\n"));
        std::cout << "esalStpPortCreate fail" << esalSaiError(retcode) << "\n";
        return false;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_STP_PORT_ATTR_STP;
    attr.value.oid = stpSai;
    attributes.push_back(attr);
    
    attr.id = SAI_STP_PORT_ATTR_BRIDGE_PORT;
    attr.value.oid = bridgePortSai;
    attributes.push_back(attr);

    attr.id = SAI_STP_PORT_ATTR_STATE;
    attr.value.s32 = SAI_STP_PORT_STATE_FORWARDING;
    attributes.push_back(attr);

    retcode = saiStpApi->create_stp_port(stpPortSai, esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "STP object creation fails in esalStpPortCreate\n"));
        std::cout << "esalStpPortCreate fail" << esalSaiError(retcode) << "\n";
        return false;
    }
    // Update the stp port table in the shadow.  Then bump counter.
    //
    StpGroupMember mbr; 
    mbr.bridgePortSai = bridgePortSai;
    mbr.stpSai = stpSai;
    mbr.stpPortSai = *stpPortSai;
    if (!esalFindBridgePortId(bridgePortSai, &mbr.portId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalFindBridgePortId fail esalStpPortCreate\n"));
        std::cout << "can't find portid for bridgePortSai:" << bridgePortSai << "\n";
              return ESAL_RC_FAIL;    
    }
    stpPortTable.push_back(mbr);
#endif

    return true;   
}

static bool serializeStpTableConfig(const std::vector<StpGroupMember>& stpPortTable,
                                    const std::string& fileName) {
    std::unique_lock<std::mutex> lock(stpTableMutex);

    libconfig::Config cfg;
    libconfig::Setting& root = cfg.getRoot();

    libconfig::Setting& stpTableSetting =
            root.add("stpPortTable", libconfig::Setting::TypeList);

    for (const auto& stpMember : stpPortTable) {
        uint32_t lPort;
        if (!saiUtils.GetLogicalPort(0, stpMember.portId, &lPort)) {
            continue;
        }
        libconfig::Setting& stpEntry =
                stpTableSetting.add(libconfig::Setting::TypeGroup);
        stpEntry.add("portId",        libconfig::Setting::TypeInt)   = stpMember.portId;
        stpEntry.add("stpSai",        libconfig::Setting::TypeInt64) = static_cast<int64_t>(stpMember.stpSai);
        stpEntry.add("bridgePortSai", libconfig::Setting::TypeInt64) = static_cast<int64_t>(stpMember.bridgePortSai);
        stpEntry.add("stpPortSai",    libconfig::Setting::TypeInt64) = static_cast<int64_t>(stpMember.stpPortSai);
        stpEntry.add("stpState",      libconfig::Setting::TypeInt)   = static_cast<int>(stpMember.stpState);
    }

    try {
        cfg.writeFile(fileName.c_str());
        return true;
    } catch (const libconfig::FileIOException& ex) {
        std::cout << "Error writing to file: " << ex.what() << std::endl;
        return false;
    }
}

static bool deserializeStpTableConfig(std::vector<StpGroupMember>& stpPortTable, const std::string& fileName) {
    libconfig::Config cfg;
    try {
        cfg.readFile(fileName.c_str());
    } catch (const libconfig::FileIOException& ex) {
        std::cout << "Error reading file: " << ex.what() << std::endl;
        return false;
    } catch (const libconfig::ParseException& ex) {
        std::cout << "Error parsing file: " << ex.what() << " at line "
                  << ex.getLine() << std::endl;
        return false;
    }

    libconfig::Setting& portTableSetting = cfg.lookup("stpPortTable");
    if (!portTableSetting.isList()) {
        std::cout << "portTable is not a list" << std::endl;
        return false;
    }

    stpPortTable.clear();
    for (int i = 0; i < portTableSetting.getLength(); ++i) {
        libconfig::Setting& portEntry = portTableSetting[i];

        int portId, stpState;
        long long stpSai, bridgePortSai, stpPortSai;

        if (!(portEntry.lookupValue("portId", portId)               &&
              portEntry.lookupValue("stpSai", stpSai)               &&
              portEntry.lookupValue("bridgePortSai", bridgePortSai) &&
              portEntry.lookupValue("stpPortSai", stpPortSai)       &&
              portEntry.lookupValue("stpState", stpState))) {
            return false;
        }

        StpGroupMember member;
        member.portId = static_cast<uint16_t>(portId);
        member.stpSai = static_cast<sai_object_id_t>(stpSai);
        member.bridgePortSai = static_cast<sai_object_id_t>(bridgePortSai);
        member.stpPortSai = static_cast<sai_object_id_t>(stpPortSai);
        member.stpState = static_cast<vendor_stp_state_t>(stpState);
        stpPortTable.push_back(member);
    }

    return true;
}

static void printStpGroupMember(const StpGroupMember& stpMember) {
    std::cout << "Port ID: " << std::dec << stpMember.portId
        << ", STP OID: 0x" << std::setw(16) << std::setfill('0') << std::hex << stpMember.stpSai
        << ", Bridge Port OID: 0x" << std::setw(16) << std::setfill('0') << std::hex << stpMember.bridgePortSai
        << ", STP Port OID: 0x" << std::setw(16) << std::setfill('0') << std::hex << stpMember.stpPortSai
        << ", STP State: " << stpMember.stpState
        << std::endl;
}

bool stpWarmBootSaveHandler() {
    return serializeStpTableConfig(stpPortTable, BACKUP_FILE_STP);
}

bool stpWarmBootRestoreHandler() {
    bool status = true;

    std::vector<StpGroupMember> stpTable;
    status = deserializeStpTableConfig(stpTable, BACKUP_FILE_STP);
    if (!status) {
        std::cout << "Error deserializing STP table" << std::endl;
        return false;
    }

    if (stpTable.empty()) {
        std::cout << "STP table is empty!" << std::endl;
        return true;
    }

    std::cout << "Found STP configurations:" << std::endl;
    for (const auto& mbr : stpTable) {
        printStpGroupMember(mbr);
    }

    std::cout << std::endl;
    std::cout << "Restore process:" << std::endl;

    for (const auto& mbr : stpTable) {
        uint32_t lport;
        if (!saiUtils.GetLogicalPort(0, mbr.portId, &lport)) {
            std::cout << "stpWarmBootRestoreHandler failed to get lPort"
                        << " pPort=" << mbr.portId << std::endl;
            return false;
        }
        if (VendorSetPortStpState(lport, mbr.stpState) != ESAL_RC_OK) {
            std::cout << "Error setting STP state for port " << mbr.portId << std::endl;
            return false;
        }
    }

    return true;
}

void stpWarmBootCleanHandler() {
    std::unique_lock<std::mutex> lock(stpTableMutex);
    stpPortTable.clear();
}

}
