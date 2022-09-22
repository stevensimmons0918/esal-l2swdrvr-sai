/*!
 * @file      esalSaiUtils.cc
 * @copyright (C) 2015 Fujitsu Network Communications, Inc.
 * @brief     ESAL Utils class to read common attributes
 * @author    smanjuna
 * @date      4/2019
 * Document Reference :
 */

#include "esal-vendor-api/headers/esalSaiUtils.h"
#include <sys/stat.h>
#include <fstream>
#include <algorithm>
#include "lib/swerr.h"

EsalSaiUtils::EsalSaiUtils() {
    unitCode_ = GetPsiUnitCode();
    fwdlType_ = GetPsiFwdlType();
    cfgPath_ = GetCfgPath("sai");
    std::cout << __FUNCTION__ << " cfgPath=" << cfgPath_ << std::endl;
    if (!cfgPath_.empty()) {
        cfg_ = new Libcfg(cfgPath_);
    } else {
        cfg_ = nullptr;
        Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cfgPath_ empty\n"));
    }

    if (cfg_ != nullptr) {
        if (cfg_->getStatus() != Libcfg::LcStatus::READ) {
            delete cfg_;
            cfg_ = nullptr;
            Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "No sai.cfg to configure\n"));
        } else {
            ParseConfig();
        }
    }
}

EsalSaiUtils::~EsalSaiUtils() {
    if (cfg_ != nullptr) {
        delete cfg_;
        cfg_ = nullptr;
    }
}

std::string EsalSaiUtils::GetUnitCode(void) {
    return unitCode_;
}

std::string EsalSaiUtils::GetFwdlType(void) {
    return fwdlType_;
}

std::string EsalSaiUtils::GetPsiUnitCode() {
    std::string uc("UNKNOWN");
    const char* buf = NULL;
    if ((buf = getenv("PSI_unitCode")) != NULL) {
        uc = buf;
    }
    return uc;
}

std::string EsalSaiUtils::GetPsiFwdlType() {
    std::string fwdlType("UNKNOWN");
    const char* buf = NULL;
    if ((buf = getenv("PSI_fwdlType")) != NULL) {
        fwdlType = buf;
    }
    return fwdlType;
}

std::string EsalSaiUtils::GetCfgPath(const std::string &name) {
#ifndef UTS
    std::string cfgBasePath = "/usr/local/fnc/esalbase";
#else
    std::string cfgBasePath = SRCPATH + std::string("/test");
#endif
    std::string basePath = cfgBasePath + std::string("/") + unitCode_;
    std::string fwdlPath = basePath + "/" + fwdlType_;
    std::string path;
    // Check for existence of this subdir
    struct stat fs;
    if ((::stat(fwdlPath.c_str(), &fs)) == 0) {
        basePath = fwdlPath;  // if fwdlType dir exists, add to cfg_path
    }

    path = basePath + "/" + name + ".cfg";
    if (FILE *file1 = fopen(path.c_str(), "r")) {
        fclose(file1);
    } else {
        path = "";
    }
    return path;
}

bool EsalSaiUtils::GetPhysicalPortInfo(const uint32_t lPort,
                        uint32_t *devId, uint32_t *pPort) {
    bool rc = true;

    if (devId == nullptr) {
        rc =false;
        Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "devId nullptr\n"));
    } else if (pPort == nullptr) {
        rc =false;
        Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "pPort nullptr\n"));
    } else if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        rc =false;
        std::string err = "lPort not in phyPortInfoMap_" +
                          std::string(" lPort=") +
                          std::to_string(lPort) + "\n";
        Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    } else {
        *devId = phyPortInfoMap_[lPort].devId;
        *pPort = phyPortInfoMap_[lPort].pPort;
    }

    return rc;
}

bool EsalSaiUtils::GetLogicalPort(const uint32_t devId,
                        const uint32_t pPort, uint32_t *lPort) {
    bool rc = false;

    if (lPort == nullptr) {
        Swerr::generate(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "lPort nullptr\n"));
    } else {
        for (auto const& p : phyPortInfoMap_) {
            if ((p.second.devId == devId) && (p.second.pPort == pPort)) {
                *lPort = p.first;
                rc = true;
                break;
            }
        }
    }

    return rc;
}

void EsalSaiUtils::ParseConfig(void) {
    std::string key = "ports";
    const libconfig::Setting &set = cfg_->getConfigSetting(key.c_str());
    bool done = false;

    for (int i = 0; !done; i++) {
        try {
            PhyPortInfo portInfo;
            uint32_t lPort = set[i]["logicalPort"];
            portInfo.pPort = set[i]["physicalPort"];
            portInfo.devId = set[i]["devId"];
            phyPortInfoMap_[lPort] = portInfo;

            std::cout << __FUNCTION__ << ":" << __LINE__
                      << " lPort=" << lPort
                      << " devId=" << portInfo.devId
                      << " pPort=" << portInfo.pPort << std::endl;
        } catch(libconfig::SettingTypeException &e) {
            done = true;
            std::cout << "catch settingType " << e.what() << std::endl;
        } catch(libconfig::SettingNotFoundException &e) {
            done = true;
            std::cout << "catch settingNotFound " << e.what() << std::endl;
        }
    }
}