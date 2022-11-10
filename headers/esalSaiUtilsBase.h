/*!
 * @file      esalUtilsBase.h
 * @copyright (C) 2015 Fujitsu Network Communications, Inc.
 * @brief     ESAL Utils class to read common attributes
 * @author    smanjuna
 * @date      9/2022
 * Document Reference :
 */

#ifndef ESAL_VENDOR_API_HEADERS_ESALUTILSBASE_H_
#define ESAL_VENDOR_API_HEADERS_ESALUTILSBASE_H_

#include <iostream>
#include <string>
#include <vector>

class EsalSaiUtilsBase {
 public:
    //! Constructor
    EsalSaiUtilsBase() {
    }

    virtual ~EsalSaiUtilsBase() {
    }

    virtual std::string GetUnitCode(void) = 0;

    virtual std::string GetFwdlType(void) = 0;

    virtual std::string GetCfgPath(const std::string &name) = 0;

    virtual bool GetPhysicalPortInfo(const uint32_t lPort,
                        uint32_t *devId, uint32_t *pPort) = 0;

    virtual bool GetLogicalPort(const uint32_t devId,
                        const uint32_t pPort, uint32_t *lPort) = 0;

    virtual bool GetLogicalPortList(const uint32_t devId,
                                    std::vector<uint32_t> *list) = 0;
};

#endif  // ESAL_VENDOR_API_HEADERS_ESALUTILSBASE_H_
