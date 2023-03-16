/**
 * @file      esalSaiPort.cc
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
#include <string>
#include <cinttypes>
#include <esal_vendor_api/esal_vendor_api.h>

#ifndef LARCH_ENVIRON
#include "pf_proto/esal_pm.pb.h" 
#endif
#include "sai/sai.h"
#include "sai/saistp.h"

extern "C" {

void esalDumpPortTable(void);
int VendorRcToString(int rc, char *strErr) {
    std::cout << __PRETTY_FUNCTION__ << " " << rc  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    const int MaxStrLen = 64; // FIXME: Change parm to (const char*) *strErr;

    const char *retStr = "Unknown Reason";
    switch (rc) {
        case ESAL_RC_OK:
            retStr = "OK";
            break;
        case ESAL_RC_FAIL:
            retStr = "Failure";
            break;
        case ESAL_SAI_FAIL:
            retStr = "Switch API Failure";
            break;
        case ESAL_SFP_FAIL:
            retStr = "SFP Lib Failure";
            break;
            break;
        case ESAL_RESOURCE_EXH:
            retStr = "Resource Exhaustion";
            break;
        case ESAL_INVALID_PORT:
            retStr = "Invalid Port";
            break;
        case ESAL_INVALID_VLAN:
            retStr = "Invalid VLAN";
            break;
        default:
            break;
    }

    strncpy(strErr, retStr, MaxStrLen);
    strErr[MaxStrLen-1] = 0;
    return ESAL_RC_OK;
}

const char *esalSaiError(sai_status_t rc) {
#ifndef UTS
    switch (rc) {
        case SAI_STATUS_SUCCESS: return "Success";
        case SAI_STATUS_FAILURE: return "Failure";
        case SAI_STATUS_NOT_SUPPORTED: return "Not Supported";
        case SAI_STATUS_NO_MEMORY: return "No Memory";
        case SAI_STATUS_INSUFFICIENT_RESOURCES:
                                   return "Insufficient Resources";
        case SAI_STATUS_INVALID_PARAMETER: return "Invalid Parameter";
        case SAI_STATUS_ITEM_ALREADY_EXISTS: return "Item Already Exists";
        case SAI_STATUS_ITEM_NOT_FOUND: return "Item Not Found";
        case SAI_STATUS_BUFFER_OVERFLOW: return "Buffer Overflow";
        case SAI_STATUS_INVALID_PORT_NUMBER: return "Invalid Port Number";
        case SAI_STATUS_INVALID_PORT_MEMBER: return "Invalid Port Member";
        case SAI_STATUS_INVALID_VLAN_ID: return "Invalid VLAN ID";
        case SAI_STATUS_UNINITIALIZED: return "Uninitialized";
        case SAI_STATUS_TABLE_FULL: return "Table Full";
        case SAI_STATUS_MANDATORY_ATTRIBUTE_MISSING:
                                    return "Mandatory Attr Mising";
        case SAI_STATUS_NOT_IMPLEMENTED: return "Not Implemented";
        case SAI_STATUS_ADDR_NOT_FOUND: return "Address Not Found";
        case SAI_STATUS_OBJECT_IN_USE: return "Object Id In Use";
        case SAI_STATUS_INVALID_OBJECT_TYPE: return "Invalid Object Type";
        case SAI_STATUS_INVALID_OBJECT_ID : return "Invalid Object Id";
        case SAI_STATUS_INVALID_NV_STORAGE: return "Invalid NV Storage";
        case SAI_STATUS_NV_STORAGE_FULL: return "NV Storage Full";
        case SAI_STATUS_SW_UPGRADE_VERSION_MISMATCH:
                                         return "Upgrade Version Mismatch";
        case SAI_STATUS_NOT_EXECUTED: return "Status Not Executed";
        case SAI_STATUS_INVALID_ATTRIBUTE_0: return "Invalid Attribute 0";
        case SAI_STATUS_INVALID_ATTRIBUTE_MAX: return "Invalid Attribute Max";
        case SAI_STATUS_INVALID_ATTR_VALUE_0:
                                          return "Invalid Attribute Value 0";
        case SAI_STATUS_INVALID_ATTR_VALUE_MAX:
                                          return "Invalid Attribute Value Max";
        case SAI_STATUS_ATTR_NOT_IMPLEMENTED_0: return "Not Implemented";
        case SAI_STATUS_ATTR_NOT_IMPLEMENTED_MAX: return "Not Implemented Max";
        case SAI_STATUS_UNKNOWN_ATTRIBUTE_0: return "Unknown Attribute 0";
        case SAI_STATUS_UNKNOWN_ATTRIBUTE_MAX: return "Unknown Attribute Max";
        case SAI_STATUS_ATTR_NOT_SUPPORTED_0: return "Attribute Supported 0";
        case SAI_STATUS_ATTR_NOT_SUPPORTED_MAX:
                                          return "Attribute Supported Max";
        default:
            break;
    }
#endif
    return "Unknown return code";
}

int VendorGetL2Pm(uint16_t *usedLen, uint16_t maxLen, char* gpbBuf) {
    int rc = ESAL_RC_OK;
    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }
#ifdef UTS
    rc = ESAL_RC_FAIL;
#else
#ifndef LARCH_ENVIRON
    VendorEsalPmBuf msg;


    // Unpack the message to determine for PMs.
    std::string buffer;
    buffer.assign(gpbBuf, *usedLen);
    if (!msg.ParseFromString(buffer)) {
        std::cout << "msg is null" << msg.DebugString() << std::endl;
        rc = ESAL_RESOURCE_EXH;
    } else {
        // Get the API for port.
        sai_status_t retcode;
        sai_port_api_t *saiPortApi;
        retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
        if (retcode) {
            std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                      << std::endl;
            return ESAL_SAI_FAIL;
        }

        // Iterate through all of the ports.
        uint8_t numBufs = msg.pm_buffers_size();
        for (uint8_t i = 0; i < numBufs; ++i) {
            // Allocate a cleared buffer.
            VendorEsalPm* vendorPmi = msg.mutable_pm_buffers(i);
            if (!vendorPmi) {
                std::cout << "NO PMI buff\n";
                return ESAL_RESOURCE_EXH;
            }
            PfEsalPmCounters *pmCtrs = new PfEsalPmCounters();
            if (!pmCtrs) {
                std::cout << "ESAL Counters Exhausted\n";
                return ESAL_RESOURCE_EXH;
            }
            vendorPmi->set_allocated_counters(pmCtrs);

            // Get physical port
            uint32_t dev;
            uint32_t pPort;
            if (!saiUtils.GetPhysicalPortInfo(
                        vendorPmi->port(), &dev, &pPort)) {
                std::cout << "Failed to get pPort, lPort=" << vendorPmi->port()
                          << std::endl;
                continue;
            }
            // Check to see port exists.
            sai_object_id_t portSai;
            if (!esalPortTableFindSai(pPort, &portSai)) {
                continue;
            }
            // Get the stats
            const int numCtrs = 16;
            uint64_t ctrs[numCtrs];
            sai_stat_id_t ctrIds[numCtrs];
            ctrIds[0] = SAI_PORT_STAT_IF_IN_OCTETS;
            ctrIds[1] = SAI_PORT_STAT_IF_IN_ERRORS;
            ctrIds[2] = SAI_PORT_STAT_IF_IN_UCAST_PKTS;
            ctrIds[3] = SAI_PORT_STAT_IF_IN_NON_UCAST_PKTS;
            ctrIds[4] = SAI_PORT_STAT_IF_OUT_UCAST_PKTS;
            ctrIds[5] = SAI_PORT_STAT_IF_OUT_NON_UCAST_PKTS;
            ctrIds[6] = SAI_PORT_STAT_IF_IN_BROADCAST_PKTS;
            ctrIds[7] = SAI_PORT_STAT_IF_IN_MULTICAST_PKTS;
            ctrIds[8] = SAI_PORT_STAT_IF_IN_DISCARDS;
            ctrIds[9] = SAI_PORT_STAT_IF_OUT_BROADCAST_PKTS;
            ctrIds[10] = SAI_PORT_STAT_IF_OUT_MULTICAST_PKTS;
            ctrIds[11] = SAI_PORT_STAT_IF_OUT_DISCARDS;
            ctrIds[12] = SAI_PORT_STAT_IF_OUT_OCTETS;
            ctrIds[13] = SAI_PORT_STAT_IF_OUT_ERRORS;
            ctrIds[14] = SAI_PORT_STAT_PAUSE_RX_PKTS;
            ctrIds[15] = SAI_PORT_STAT_PAUSE_TX_PKTS;

            retcode = saiPortApi->get_port_stats(
                                    portSai, numCtrs, ctrIds, ctrs); 
            if (retcode) {
               std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                         << std::endl;
               continue;
            }

            pmCtrs->set_goodrxoctets(ctrs[0]);
            pmCtrs->set_errorrxframes(ctrs[1]);
            pmCtrs->set_goodrxframes(ctrs[2] + ctrs[3]);
            pmCtrs->set_goodtxframes(ctrs[4] + ctrs[5]);
            pmCtrs->set_snmpifinucastpkts(ctrs[2]);
            pmCtrs->set_snmpifinerrors(ctrs[1]);
            pmCtrs->set_snmpifinbroadcastpkts(ctrs[6]);
            pmCtrs->set_snmpifinmulticastpkts(ctrs[7]);
            pmCtrs->set_snmpifindiscards(ctrs[8]);
            pmCtrs->set_snmpdot3inpauseframes(ctrs[14]);
            pmCtrs->set_snmpifinoctets(ctrs[0]);
            pmCtrs->set_snmpifoutucastpkts(ctrs[4]);
            pmCtrs->set_snmpifoutbroadcastpkts(ctrIds[9]);
            pmCtrs->set_snmpifoutmulticastpkts(ctrIds[10]);
            pmCtrs->set_snmpdot3outpauseframes(ctrs[15]);
            pmCtrs->set_snmpifoutdiscards(ctrIds[11]);
            pmCtrs->set_snmpifoutoctets(ctrIds[12]);
            pmCtrs->set_snmpifouterrors(ctrIds[13]);

            // Cleaar the stats.
            retcode = saiPortApi->clear_port_stats(portSai, numCtrs, ctrIds);
            if (retcode) {
               std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                         << std::endl;
            }
        }

        // Make sure that we do not exceed buffer length.
        buffer.clear();
        if (!msg.SerializeToString(&buffer)) {
            rc = ESAL_RC_OK;
        }

        *usedLen = (uint16_t) buffer.length();
        if (*usedLen < maxLen) {
            buffer.copy(gpbBuf, *usedLen);
        } else {
            std::cout << "msg is longer than max" << std::endl;
            rc = ESAL_RESOURCE_EXH;
        }
    }
#endif
#endif    
    return rc;
}

}
