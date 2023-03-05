/**
 * @file      esalSaiTag.cc
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
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <map>
#include <mutex>
#include <string>
#include <cinttypes>

#include "esal_vendor_api/esal_vendor_api.h"
#include "esal_warmboot_api/esal_warmboot_api.h"

#include <libconfig.h++>
#include <utility>

#ifndef UTS
extern "C" {
#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saivlan.h"
}
#endif

extern "C" {

struct PortTagMember {
    vendor_dtag_mode dtag_mode;
    vendor_nni_mode_t nni_mode;
};

static std::map<uint16_t, PortTagMember> portsTagMap;

static std::mutex tagMutex;

int VendorSetPortDoubleTagMode(uint16_t lPort, vendor_dtag_mode mode) {
    (void) mode;
    uint32_t dev;
    uint32_t pPort;

    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << " " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

	portsTagMap[lPort].dtag_mode = mode;

    return ESAL_RC_OK;
}

int VendorGetPortDoubleTagMode(uint16_t lPort, vendor_dtag_mode *mode) {
    (void) mode;
    uint32_t dev;
    uint32_t pPort;

    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << " " << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK; }
    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << __PRETTY_FUNCTION__ << " Failed to get pPort, "
                  << "lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    return ESAL_RC_OK;
}

int VendorSetPortNniMode(uint16_t lPort, vendor_nni_mode_t mode) {
    std::cout << __PRETTY_FUNCTION__ << lPort <<  " is NYI : FIXME " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorTagPacketsOnIngress, failed to get pPort"
            << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    switch (mode) {
        case VENDOR_NNI_MODE_UNI:
            // Set port to UNI mode.
            // In this mode port should push tag on ingress
            // regardless the tags.
            if (esalVlanAddPortTagPushPop(pPort, true, true) != ESAL_RC_OK) {
                std::cout << "VendorSetPortNniMode fail pPort: " << pPort << "\n";
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "invalid port in VendorSetPortNniMode\n"));
                return ESAL_RC_FAIL;
            }

            if (esalVlanAddPortTagPushPop(pPort, false, false) != ESAL_RC_OK) {
                std::cout << "VendorSetPortNniMode fail pPort: " << pPort << "\n";
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "invalid port in VendorSetPortNniMode\n"));
                return ESAL_RC_FAIL;
            }
            break;
        case VENDOR_NNI_MODE_NNI:
            break;
        case VENDOR_NNI_MODE_ENI:
            break;
        default:
            std::cout << "VendorSetPortNniMode fail. Wrong mode\n";
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "invalid mode in VendorSetPortNniMode\n"));
            return ESAL_RC_FAIL;
            break;
    }

	portsTagMap[lPort].nni_mode = mode;

    return ESAL_RC_OK;
}

int VendorGetPortNniMode(uint16_t port, vendor_nni_mode_t *mode) {
    std::cout << __PRETTY_FUNCTION__ << port <<  " is NYI : FIXME " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    return ESAL_RC_OK;
}

static bool restorePortsTag(std::map<uint16_t, PortTagMember>& portsTagMap) {
    bool status = true;
    int ret = ESAL_RC_OK;

    for (auto& portTag : portsTagMap) {
        uint16_t lPort = portTag.first;
        PortTagMember portTagMember = portTag.second;

        // Set Double Tag Mode
        if ((ret = VendorSetPortDoubleTagMode(lPort, portTagMember.dtag_mode)) != ESAL_RC_OK) {
            status &= false;
            std::cout << "Error setting double tag mode to " << portTagMember.dtag_mode << " for port "
                      << lPort << " err " << esalSaiError(ret) << std::endl;
            continue;
        }

        // Set NNI Tag Mode
        if ((ret = VendorSetPortNniMode(lPort, portTagMember.nni_mode)) != ESAL_RC_OK) {
            status &= false;
            std::cout << "Error setting nni tag mode to " << portTagMember.dtag_mode << " for port "
                      << lPort << " err " << esalSaiError(ret) << std::endl;
            continue;
        }
    }
    return status;
}

static bool serializeTagMapConfig(const std::map<uint16_t, PortTagMember> &portsTagMap, const std::string &fileName) {
    std::unique_lock<std::mutex> lock(tagMutex);

    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    libconfig::Setting &portsTagMapSetting = root.add("portsTagMap", libconfig::Setting::TypeList);

    for (const auto &portTag : portsTagMap) {
        libconfig::Setting &vlanEntry = portsTagMapSetting.add(libconfig::Setting::TypeGroup);
        vlanEntry.add("portId", libconfig::Setting::TypeInt) = portTag.first;
        vlanEntry.add("dtagMode", libconfig::Setting::TypeInt) = portTag.second.dtag_mode;
        vlanEntry.add("nniMode", libconfig::Setting::TypeInt) = portTag.second.nni_mode;
    }

    try {
        cfg.writeFile(fileName.c_str());
        return true;
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error writing to file: " << ex.what() << std::endl;
        return false;
    }
}

static bool deserializeTagMapConfig(std::map<uint16_t, PortTagMember> &portsTagMap, const std::string &fileName) {
    libconfig::Config cfg;
    try {
        cfg.readFile(fileName.c_str());
    } catch (const libconfig::FileIOException &ex) {
        std::cout << "Error reading file: " << ex.what() << std::endl;
        return false;
    } catch (const libconfig::ParseException &ex) {
        std::cout << "Error parsing file: " << ex.what() << " at line " << ex.getLine() << std::endl;
        return false;
    }

    libconfig::Setting &portsTagMapSetting = cfg.lookup("portsTagMap");
    if (!portsTagMapSetting.isList()) {
        std::cout << "portsTagMap is not a list" << std::endl;
        return false;
    }

    portsTagMap.clear();
    for (int i = 0; i < portsTagMapSetting.getLength(); ++i) {
        libconfig::Setting &portsTag = portsTagMapSetting[i];

        int portId;
        int dtagMode;
        int nniMode;

        if (!(portsTag.lookupValue("portId", portId) &&
              portsTag.lookupValue("dtagMode", dtagMode) &&
              portsTag.lookupValue("nniMode", nniMode))) {
            return false;
        }

        portsTagMap[portId].dtag_mode = static_cast<vendor_dtag_mode>(dtagMode);
        portsTagMap[portId].nni_mode = static_cast<vendor_nni_mode_t>(nniMode);
    }

    return true;
}

static void printVlanEntry(uint16_t num, const PortTagMember& portTag) {
    std::cout << "lPort: "      << std::dec << num
              << " dtagMode: "  << portTag.dtag_mode
              << " nniMode: "   << portTag.nni_mode
              << std::endl;
}

bool tagWarmBootSaveHandler() {
    return serializeTagMapConfig(portsTagMap, BACKUP_FILE_TAG);
}

bool tagWarmBootRestoreHandler() {
    std::map<uint16_t, PortTagMember> portsTagMap;

    bool status = true;

    status = deserializeTagMapConfig(portsTagMap, BACKUP_FILE_TAG);
    if (!status) {
        std::cout << "Error deserializing tag map" << std::endl;
        return false;
    }

    if (!portsTagMap.size()) {
        std::cout << "Tag map is empty!" << std::endl;
        return true;
    }

    std::cout << "Founded tag configurations:" << std::endl;
    for (const auto& portTag : portsTagMap) {
        printVlanEntry(portTag.first, portTag.second);
    }

    std::cout << std::endl;
    std::cout << "Restore process:" << std::endl;
    status = restorePortsTag(portsTagMap);
    if (!status) {
        std::cout << "Error restore tags" << std::endl;
        return false;
    }

    return true;
}

void tagWarmBootCleanHandler() {
    std::unique_lock<std::mutex> lock(tagMutex);
    portsTagMap.clear();
}

}


