/*!
 * @file      esalSaiUtils.h
 * @copyright (C) 2015 Fujitsu Network Communications, Inc.
 * @brief     ESAL Utils class to read common attributes
 * @author    smanjuna
 * @date      9/2022
 * Document Reference :
 */

#ifndef ESAL_VENDOR_API_HEADERS_ESALSAIUTILS_H_
#define ESAL_VENDOR_API_HEADERS_ESALSAIUTILS_H_
#ifndef LARCH_ENVIRON
#include "esal-vendor-api/headers/esalSaiUtilsBase.h"
#else
#include "headers/esalSaiUtilsBase.h"
#endif
#include <iostream>
#include <string>
#include <map>
#ifndef LARCH_ENVIRON
#include "threadutils/libcfg.h"
#endif

class EsalSaiUtils: public EsalSaiUtilsBase {
 public:
#ifdef UTS
    friend class EsalSaiUtils_test_GetUnitCode_Test;
    friend class EsalSaiUtils_test_GetFwdlType_Test;
    friend class EsalSaiUtils_test_GetPsiUnitCode1_Test;
    friend class EsalSaiUtils_test_GetPsiUnitCode2_Test;
    friend class EsalSaiUtils_test_GetPsiFwdlType1_Test;
    friend class EsalSaiUtils_test_GetPsiFwdlType2_Test;
    friend class EsalSaiUtils_test_ParseConfig1_Test;
    friend class EsalSaiUtils_test_GetPhysicalPortInfo1_Test;
    friend class EsalSaiUtils_test_GetSerdes_Test;
#endif
    typedef struct {
        bool     has_vals;
        uint32_t post;
        uint32_t pre;
        uint32_t pre3;
        uint32_t atten;
        uint32_t pre2;
    } serdesTx_t;

    typedef struct {
        bool     has_vals;
        uint32_t DC;
        uint32_t LF;
        uint32_t sqlch;
        uint32_t HF;
        uint32_t BW;
    } serdesRx_t;

    typedef struct {
        bool has_vals = false;
        bool inbandEnable = false;
        bool duplexEnable = false;
        bool speedEnable = false;
        bool byPassEnable = false;
        bool flowCtrlEnable = false;
        bool flowCtrlPauseAdvertiseEnable = false;
        bool flowCtrlAsmAdvertiseEnable = false;
        bool readyToUpdFlag = false;
    } flowCtrlAttrs;

    //! Constructor
    EsalSaiUtils();

    //! Destructor
    virtual ~EsalSaiUtils();

    /*!
     * @brief           Get PSI unit code value
     * @param void
     * @return          unit code
     */
    std::string GetUnitCode(void);

    /*!
     * @brief           Get PSI fwdl type value
     * @param void
     * @return          unit code
     */
    std::string GetFwdlType(void);

    /*!
     * @brief           Get Config Path
     * @param string    filename
     * @return string   path
     */
    std::string GetCfgPath(const std::string &name);

    /*!
     * @brief           Get Physical port info for
     *                  the given logical port
     * @param lPort     Logical port number
     * @param dev       returned chip number
     * @param pPort     returned physical port number
     * @return bool     true if success, false otherwise
     */
    bool GetPhysicalPortInfo(const uint32_t lPort,
                             uint32_t *devId, uint32_t *pPort);

    /*!
     * @brief           Get Logical port info for
     *                  the given physical port
     * @param pPort     Physical port number
     * @param dev       chip number
     * @param lPort     returned logiical port number
     * @return bool     true if success, false otherwise
     */
    bool GetLogicalPort(const uint32_t devId,
                        const uint32_t pPort, uint32_t *lPort);


    /*!
     * @brief           Get SERDES info for the given logical port
     * @param lPort     Logical port number
     * @param dev       returned chip number
     * @param pPort     returned physical port number
     * @param tx        SERDES TX port settings
     * @param rx        SERDES RX port settings
     * @return bool     Have valid logical port mapping info
     */
    bool GetSerdesInfo(const uint32_t lPort,
                       uint32_t &devId, uint32_t &pPort,
                       serdesTx_t &tx, serdesRx_t &rx);

    /*!
     * @brief           Get list of all supported logical ports
     * @param devId     chip number
     * @param list      returned list of logical port numbers
     * @return bool     true if success, false otherwise
     */
    bool GetLogicalPortList(const uint32_t devId,
                            std::vector<uint32_t> *list);

    bool GetFlowCtrlAttr(
     uint32_t lPort, uint32_t &devId, uint32_t &pPort, flowCtrlAttrs &fc);

 private:
    typedef struct {
        uint32_t devId;
        uint32_t pPort;
        serdesTx_t serdesTx;
        serdesRx_t serdesRx;
        flowCtrlAttrs flowCtrl; 
    } PhyPortInfo;

    //! UnitCode
    std::string unitCode_;

    //! fwdlType
    std::string fwdlType_;

    //! cfg file path
    std::string cfgPath_;
#ifndef LARCH_ENVIRON
    //! Libcfg ptr
    Libcfg *cfg_;
#endif
    //! Map to hold logical to physical port map
    std::map<uint32_t, PhyPortInfo> phyPortInfoMap_;

    /*!
     * @brief       Function to read PSI env variable
     * @param void
     * @return      unitcode
     */
    std::string GetPsiUnitCode();

    /*!
     * @brief       Function to read PSI env variable
     * @param void
     * @return      fwdlType
     */
    std::string GetPsiFwdlType();

    //! Function to parse sai.cfg file
    void ParseConfig(void);

    //! copy constructor
    EsalSaiUtils(EsalSaiUtils const&) = delete;

    //! Operator =
    EsalSaiUtils& operator=(EsalSaiUtils const&) = delete;
};

#endif  // ESAL_VENDOR_API_HEADERS_ESALSAIUTILS_H_
