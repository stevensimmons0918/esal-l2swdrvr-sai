/*!
 * @file      esalSaiUtils.cc
 * @copyright (C) 2015 Fujitsu Network Communications, Inc.
 * @brief     ESAL Utils class to read common attributes
 * @author    smanjuna
 * @date      4/2019
 * Document Reference :
 */
#ifndef LARCH_ENVIRON
#include "esal-vendor-api/headers/esalSaiUtils.h"
#else
#include "headers/esalSaiUtils.h"
#endif
#include <sys/stat.h>
#include <fstream>
#include <algorithm>
#ifndef LARCH_ENVIRON
#include "lib/swerr.h"
#endif
#include "headers/esalSaiDef.h"

EsalSaiUtils::EsalSaiUtils() {
#ifndef LARCH_ENVIRON
    unitCode_ = GetPsiUnitCode();
    fwdlType_ = GetPsiFwdlType();
    cfgPath_ = GetCfgPath("sai.cfg");
    std::cout << __FUNCTION__ << " cfgPath=" << cfgPath_ << std::endl;
    if (!cfgPath_.empty()) {
        cfg_ = new Libcfg(cfgPath_);
    } else {
        cfg_ = nullptr;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "cfgPath_ empty\n"));
    }

    if (cfg_ != nullptr) {
        if (cfg_->getStatus() != Libcfg::LcStatus::READ) {
            delete cfg_;
            cfg_ = nullptr;
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "No sai.cfg to configure\n"));
        } else {
            ParseConfig();
        }
    }
#endif
}

EsalSaiUtils::~EsalSaiUtils() {
#ifndef LARCH_ENVIRON
    if (cfg_ != nullptr) {
        delete cfg_;
        cfg_ = nullptr;
    }
#endif
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

        // keep the unitCode uppercase to avoid
        // multiple comparisons
        transform(uc.begin(), uc.end(), uc.begin(),
            std::ptr_fun<int, int>(std::toupper));
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


    path = basePath + "/" + name;
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

#ifndef LARCH_ENVIRON
    if (devId == nullptr) {
        rc =false;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "devId nullptr\n"));
    } else if (pPort == nullptr) {
        rc =false;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "pPort nullptr\n"));
    } else if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        rc =false;
        std::string err = "lPort not in phyPortInfoMap_" +
                          std::string(" lPort=") +
                          std::to_string(lPort) + "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    } else {
        *devId = phyPortInfoMap_[lPort].devId;
        *pPort = phyPortInfoMap_[lPort].pPort;
    }
#else
    if (devId == nullptr) {
        rc =false;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "devId nullptr\n"));
    } else if (pPort == nullptr) {
        rc =false;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "pPort nullptr\n"));
    } else {
        *devId = 0;
        *pPort = lPort;
    }
#endif
    return rc;
}

bool EsalSaiUtils::GetLogicalPort(const uint32_t devId,
                        const uint32_t pPort, uint32_t *lPort) {
    bool rc = false;

#ifndef LARCH_ENVIRON
    if (lPort == nullptr) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
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
#else
    if (lPort == nullptr) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "lPort nullptr\n"));
    } else {
        *lPort = pPort;
        rc = true;
    }
#endif

    return rc;
}

bool EsalSaiUtils::GetSerdesInfo(const uint32_t lPort,
                                 uint32_t &devId, uint32_t &pPort,
                                 serdesTx_t &tx, serdesRx_t &rx)
{
    bool rc = true;

#ifndef LARCH_ENVIRON
    if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        devId = 9999;
        pPort = 9999;
        tx.has_vals = false;
        rx.has_vals = false;
        rc = false;
        std::string err = "lPort not in phyPortInfoMap_" +
            std::string(" lPort=") +
            std::to_string(lPort) + "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    }
    else {
        devId = phyPortInfoMap_[lPort].devId;
        pPort = phyPortInfoMap_[lPort].pPort;
        tx = phyPortInfoMap_[lPort].serdesTx;
        rx = phyPortInfoMap_[lPort].serdesRx;
    }
#endif
    return rc;
}

bool EsalSaiUtils::GetChangeable(const uint32_t lPort)
{
    bool rc = false;

#ifndef LARCH_ENVIRON
    if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        rc = false;
        std::string err = "lPort not in phyPortInfoMap_" +
            std::string(" lPort=") +
            std::to_string(lPort) + "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    }
    else {
        rc = phyPortInfoMap_[lPort].changeable;
    }
#endif
    return rc;
}

bool EsalSaiUtils::GetL2CommsProvDisable(const uint32_t lPort)
{
    bool rc = false;

#ifndef LARCH_ENVIRON
    if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        rc = false;
        std::string err = "lPort not in phyPortInfoMap_" +
            std::string(" lPort=") +
            std::to_string(lPort) + "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    }
    else {
        rc = phyPortInfoMap_[lPort].l2CommsProvDisable;
    }
#endif
    return rc;
}

bool EsalSaiUtils::GetRateLimitInfo(const uint32_t lPort,
                                    uint32_t &devId, uint32_t &pPort,
                                    rateLimit_t &rLimit) {
    bool rc = true;

#ifndef LARCH_ENVIRON
    if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) {
        rLimit.has_vals = false;
        rc = false;
        std::string err = "lPort not in phyPortInfoMap_" +
            std::string(" lPort=") +
            std::to_string(lPort) + "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err));
    }
    else {
        devId = phyPortInfoMap_[lPort].devId;
        pPort = phyPortInfoMap_[lPort].pPort;
        rLimit = phyPortInfoMap_[lPort].rateLimits;
    }
#endif
    return rc;
}
void EsalSaiUtils::ParseConfig(void) {
#ifndef LARCH_ENVIRON
    std::string key = "ports";
    const libconfig::Setting &set = cfg_->getConfigSetting(key.c_str());
    bool done = false;

    for (int i = 0; !done; i++) {
        try {
            PhyPortInfo portInfo;
            uint32_t lPort = set[i]["logicalPort"];
            portInfo.pPort = set[i]["physicalPort"];
            portInfo.devId = set[i]["devId"];

            const libconfig::Setting &portsSetting = set[i];

            // Check and populate rate limits if present
            rateLimit_t rLimit;
            memset(&rLimit, 0, sizeof(rLimit));
            rLimit.has_vals = false;
            if (portsSetting.exists("rateLimits")) {
              const libconfig::Setting &r = set[i]["rateLimits"];
              rLimit.bcastRateLimit  = set[i]["rateLimits"]["bcastRateLimit"];
              rLimit.bcastBurstLimit = r["bcastBurstLimit"];
              rLimit.mcastRateLimit  = r["mcastRateLimit"];
              rLimit.mcastBurstLimit = r["mcastBurstLimit"];
              rLimit.has_vals = true;
            }
            portInfo.rateLimits = rLimit;

            // Check for and populate serdes TX values if present
            serdesTx_t sTxVal;
            memset(&sTxVal, 0, sizeof(sTxVal));
            sTxVal.has_vals = false;
            if (portsSetting.exists("serdesTx")) {
              const libconfig::Setting &sTxSetting = portsSetting["serdesTx"];
              sTxVal.post = sTxSetting["post"];
              sTxVal.pre  = sTxSetting["pre"];
              sTxVal.pre3 = sTxSetting["pre3"];
              sTxVal.atten = sTxSetting["atten"];
              sTxVal.pre2 = sTxSetting["pre2"];
              sTxVal.has_vals = true;
            }
            portInfo.serdesTx = sTxVal;

            // Check for and populate serdes Rx values if present
            serdesRx_t sRxVal;
            memset(&sRxVal, 0, sizeof(sRxVal));
            sRxVal.has_vals = false;
            if (portsSetting.exists("serdesRx")) {
              const libconfig::Setting &sRxSetting = portsSetting["serdesRx"];
              sRxVal.DC = sRxSetting["DC"];
              sRxVal.LF = sRxSetting["LF"];
              sRxVal.sqlch = sRxSetting["sqlch"];
              sRxVal.HF = sRxSetting["HF"];
              sRxVal.BW = sRxSetting["BW"];
              sRxVal.has_vals = true;
            }
            portInfo.serdesRx = sRxVal;

            flowCtrlAttrs flowCtrl;
            flowCtrl.has_vals = false; 
            if (portsSetting.exists("flowCtrl")) {
              const libconfig::Setting &fcSetting = portsSetting["flowCtrl"];
              flowCtrl.inbandEnable = fcSetting["inbandEnable"];
              flowCtrl.duplexEnable = fcSetting["duplexEnable"];
              flowCtrl.speedEnable = fcSetting["speedEnable"];
              flowCtrl.byPassEnable = fcSetting["byPassEnable"];
              flowCtrl.flowCtrlEnable = fcSetting["flowCtrlEnable"];
              flowCtrl.flowCtrlPauseAdvertiseEnable = 
                  fcSetting["flowCtrlPauseAdvertiseEnable"];
              flowCtrl.flowCtrlAsmAdvertiseEnable = 
                  fcSetting["flowCtrlAsmAdvertiseEnable"];
              flowCtrl.has_vals = true; 
            }

            portInfo.flowCtrl = flowCtrl;

            portInfo.changeable = false; 
            if (portsSetting.exists("changeable")) {
                portInfo.changeable = set[i]["changeable"];
            }

            portInfo.l2CommsProvDisable = false;
            if (portsSetting.exists("l2CommsProvDisable")) {
                portInfo.changeable = set[i]["l2CommsProvDisable"];
            }

            std::cout << __FUNCTION__ << ":" << __LINE__
                      << " lPort=" << lPort
                      << " devId=" << portInfo.devId
                      << " pPort=" << portInfo.pPort
                      << " serdesTx.vals=" << portInfo.serdesTx.has_vals
                      << " serdesRx.vals=" << portInfo.serdesRx.has_vals
                      << " l2CommsProvDisable=" << portInfo.l2CommsProvDisable
                      << std::endl;

            phyPortInfoMap_[lPort] = portInfo;

        } catch(libconfig::SettingTypeException &e) {
            done = true;
            std::cout << "catch settingType " << e.what() << std::endl;
        } catch(libconfig::SettingNotFoundException &e) {
            done = true;
            std::cout << "catch settingNotFound " << e.what() << std::endl;
        }
    }
#endif
}

bool EsalSaiUtils::GetLogicalPortList(const uint32_t devId,
                                      std::vector<uint32_t> *list) {
    bool rc = true;

#ifndef LARCH_ENVIRON
    if (list == nullptr) {
        rc = false;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "list nullptr\n"));
    } else {
        for (auto const& p : phyPortInfoMap_) {
            if (p.second.devId == devId) {
                list->push_back(p.first);
            }
        }
    }
#endif

    return rc;
}

bool EsalSaiUtils::GetFlowCtrlAttr(
     uint32_t lPort, uint32_t &devId, uint32_t &pPort, flowCtrlAttrs &fc) {

    // Get the contents for flow control, and return failure if they have
    // not been initialized. 
    // 
    if (phyPortInfoMap_.find(lPort) == phyPortInfoMap_.end()) return false;
    if (!phyPortInfoMap_[lPort].flowCtrl.has_vals) return false; 

    devId = phyPortInfoMap_[lPort].devId;
    pPort = phyPortInfoMap_[lPort].pPort;
    fc = phyPortInfoMap_[lPort].flowCtrl;

    return true;

}

