/**
 * @file      esalSaiFdb.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. SAI FDB 
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
#include <mutex>
#include <vector>
#include <map>

#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saibridge.h"
#include "sai/saifdb.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif

#include "esal_vendor_api/esal_vendor_api.h"

// Updates to the table comes from a notification in SAI Switch which is a single 
// thread.   All updates are done in the shadow.  We have multiple readers so
// no need for mutex. At worst, an inconsistent record when some of the fields
// in an entry are temporarily not for the same update.  There is no dereferenced
// pointer so no risk of trap. 
struct FdbEntry {
    sai_mac_t macAddr;
    uint16_t egressPort;
    sai_object_id_t vlanSai;
};

const int MAX_FDB_TABLE_SIZE = 4096;
static FdbEntry fdbTable[MAX_FDB_TABLE_SIZE]; 
static int fdbTableSize = 0; 

extern "C" {

static bool findPortIdInAttr(
    uint32_t attrId, int cnt, sai_attribute_t *attr, uint16_t *portId) {
    for(auto i = 0; i < cnt; i++) {
        if (attr[i].id == attrId) {
            sai_object_id_t portSai = attr[i].value.oid; 
            return esalFindBridgePortId(portSai, portId);
        }
    }
    return false; 
}

static bool findVlanSaiInAttr(
   int cnt, sai_attribute_t *attr, sai_object_id_t *vlanSai) {
    for(auto i = 0; i < cnt; i++) {
        if (attr[i].id == SAI_FDB_FLUSH_ATTR_BV_ID) {
            *vlanSai = attr[i].value.oid; 
            return true; 
        }
    }
    return false; 
}

void esalAlterForwardingTable(sai_fdb_event_notification_data_t *fdbNotify) {
     // Verify pointer
     if (!fdbNotify) return; 

     sai_fdb_entry_t &fdbUpd = fdbNotify->fdb_entry;

     FdbEntry newEntry;
     uint16_t portId; 
     sai_object_id_t vlanSai;
     
     // Action may be either LEARN, AGE, MOVE, or FLUSH.
     switch (fdbNotify->event_type) {
         case SAI_FDB_EVENT_LEARNED:
             if (!findPortIdInAttr(
                 SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID, fdbNotify->attr_count, 
                 fdbNotify->attr, &portId)){
                return; 
             }
             for(int i = 0; i < fdbTableSize; i++) {
                 auto &fdb = fdbTable[i];
                 if ((fdb.egressPort == portId) && 
                     !memcmp(fdb.macAddr, fdbUpd.mac_address, sizeof(sai_mac_t))){
                    return;
                 }
             }
  
             if (fdbTableSize < MAX_FDB_TABLE_SIZE) {
                 memcpy(newEntry.macAddr, fdbUpd.mac_address, sizeof(sai_mac_t));
                 newEntry.egressPort = portId; 
                 newEntry.vlanSai = fdbUpd.bv_id;
                 fdbTable[fdbTableSize++] = newEntry; 
#ifdef LARCH_ENVIRON
                 std::cout << "New Mac Learned: " << std::hex << newEntry.macAddr[0]
                           << ":" << std::hex << newEntry.macAddr[1] << ":" << std::hex << newEntry.macAddr[2]
                           << ":" << std::hex << newEntry.macAddr[3] << ":" << std::hex << newEntry.macAddr[4]
                           << ":" << std::hex << newEntry.macAddr[5]
                           << ", port = " << portId << ", vlan = " << fdbUpd.bv_id
                           << "\n";
#endif
             }
             break; 

         case SAI_FDB_EVENT_AGED:
             if (!findPortIdInAttr(
                     SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID, fdbNotify->attr_count, 
                     fdbNotify->attr, &portId)){
                return; 
             }
             for(int i = 0; i < fdbTableSize; i++) {
                 auto &fdb = fdbTable[i];
                 if ((fdb.egressPort == portId) && 
                     !memcmp(fdb.macAddr, fdbUpd.mac_address, sizeof(sai_mac_t))){
                    fdbTable[i] = fdbTable[fdbTableSize-1];
                    fdbTableSize--;
                    break ;
                 }
             }
             break; 

         case SAI_FDB_EVENT_MOVE:
             if (!findPortIdInAttr(
                     SAI_FDB_ENTRY_ATTR_BRIDGE_PORT_ID, fdbNotify->attr_count, 
                     fdbNotify->attr, &portId)){
                 return; 
             }
             for(int i = 0; i < fdbTableSize; i++) {
                 auto &fdb = fdbTable[i];
                 if (fdb.macAddr == fdbUpd.mac_address) {
                    fdb.egressPort = portId; 
                    break;
                 }
             }
             break; 

         case SAI_FDB_EVENT_FLUSHED:
             if (findPortIdInAttr(
                     SAI_FDB_FLUSH_ATTR_BRIDGE_PORT_ID, fdbNotify->attr_count, 
                     fdbNotify->attr, &portId)){

                 // Handle port flush 
                 int cpyIdx = 0;
                 for(int i = 0; i < fdbTableSize; i++) {
                     auto &fdb = fdbTable[i];
                     if (fdb.egressPort != portId) { 
                         fdbTable[cpyIdx++] = fdb; 
                     }
                 }
                 fdbTableSize = cpyIdx;
             } else if (findVlanSaiInAttr(
                 fdbNotify->attr_count, fdbNotify->attr, &vlanSai)){ 

                 // Handle vlan flush 
                 std::vector<FdbEntry> tmpFdbTable; 
                 int cpyIdx = 0;
                 for(int i = 0; i < fdbTableSize; i++) {
                     auto &fdb = fdbTable[i];
                     if (fdb.vlanSai != vlanSai) { 
                         fdbTable[cpyIdx++] = fdb; 
                     }
                 }
                 fdbTableSize = cpyIdx;
             } else {
                 // Handle complete flush.
                 fdbTableSize = 0;
             }
             break; 

         default:
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                 SWERR_FILELINE, "Unknown csse in esalAlterForwardingTable\n"));
             std::cout <<  "Unknown case-esalAlterForwardingTable: "
                 << fdbNotify->event_type << std::endl;
             break; 
     }
     return; 
}

int VendorPurgeMacEntriesPerPort(uint16_t lPort) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    int rc  = ESAL_RC_OK;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorPurgeMacEntriesPerPort, failed to get pPort"
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

#ifndef UTS
    // Get the bridge API
    sai_status_t retcode;
    sai_fdb_api_t *saiFdbApi;
    retcode =  sai_api_query(SAI_API_FDB, (void**) &saiFdbApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
           SWERR_FILELINE, "API Query Fail in VendorPurgeMacEntriesPerPort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }

    // Find port sai.
    sai_object_id_t portSai;
    if (!esalFindBridgePortSaiFromPortId(pPort, &portSai)) {
        std::cout << "port_table_find_sai fail pPort:" << pPort << std::endl;
        return ESAL_RC_FAIL; 
    }

    // Create Attribute list.
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_FDB_FLUSH_ATTR_BRIDGE_PORT_ID;
    attr.value.oid = portSai;
    attributes.push_back(attr); 

    retcode = saiFdbApi->flush_fdb_entries(
                           esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "Flush FDB Fail in " \
                            "VendorPurgeMacEntriesPerPort\n"));
        std::cout << "Fail flush_fdb_entries: " << esalSaiError(retcode)
                  << std::endl;
    }
#endif

    return rc;
}

int VendorPurgeMacEntries(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_fdb_api_t *saiFdbApi;
    retcode =  sai_api_query(SAI_API_FDB, (void**) &saiFdbApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "API Query Fail in VendorPurgeMacEntries\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    retcode = saiFdbApi->flush_fdb_entries(
                    esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "Flush FDB Fail in VendorPurgeMacEntries\n"));
        std::cout << "Fail flush_fdb_entries: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    return rc;
}

int VendorGetMacTbl(uint16_t lPort, uint16_t *numMacs, unsigned char *macs) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort << std::endl;
    int rc  = ESAL_RC_OK;
    int maxMacs = ((*numMacs/sizeof(sai_mac_t)) > 256) ? 
        (*numMacs/sizeof(sai_mac_t)) : 256;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorPurgeMacEntriesPerPort, failed to get pPort"
                  << " lPort=" << lPort << std::endl;
        return ESAL_RC_FAIL;
    }
    
    *numMacs = 0; 
    int curMac = 0;
    for(int i = 0; i < fdbTableSize; i++) {
        auto &fdb = fdbTable[i];
        if (fdb.egressPort == pPort) {
            memcpy(macs+(*numMacs), fdb.macAddr, sizeof(sai_mac_t));
            (*numMacs) += sizeof(sai_mac_t); 
            if (curMac++ >= maxMacs) break;
        }
    }

    return rc;
}

}
