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

#include <iostream>

#include <string>
#include <cinttypes>
#include <mutex>
#include <vector>
#include <map>

#ifndef UTS
#include "sai/sai.h"
#include "sai/saiport.h"
#include "sai/saibridge.h"
#include "sai/saifdb.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif
#endif

#include "esal-vendor-api/esal_vendor_api.h"
#include "lib/swerr.h"


// Updates to the table comes from a notification in SAI Switch which is a single 
// thread.   All updates are done in the shadow.  We have multiple readers so
// no need for mutex. At worst, an inconsistent record when some of the fields
// in an entry are temporarily not for the same update.  There is no dereferenced
// pointer so no risk of trap. 
//
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
     //
     if (!fdbNotify) return; 


     sai_fdb_entry_t &fdbUpd = fdbNotify->fdb_entry;

     FdbEntry newEntry;
     uint16_t portId; 
     sai_object_id_t vlanSai;
     
     // Action may be either LEARN, AGE, MOVE, or FLUSH.
     //
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
                     (fdb.macAddr == fdbUpd.mac_address)) {
                    return;
                 }
             }
  
             if (fdbTableSize < MAX_FDB_TABLE_SIZE) {
                 memcpy(newEntry.macAddr, fdbUpd.mac_address, sizeof(sai_mac_t));
                 newEntry.egressPort = portId; 
                 newEntry.vlanSai = fdbUpd.bv_id;
                 fdbTable[fdbTableSize++] = newEntry; 
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
                 //
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
                 //
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
                 //
                 fdbTableSize = 0;

             }
             break; 

         default:
             SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                 SWERR_FILELINE, "Unknown csse in esalAlterForwardingTable\n"));
             std::cout << 
                 "Unknown case-esalAlterForwardingTable: " << fdbNotify->event_type << "\n";
             break; 
     }
     return; 

}

int VendorPurgeMacEntriesPerPort(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << port << std::endl;
    int rc  = ESAL_RC_OK;

#ifndef UTS
    // Get the bridge API
    //
    sai_status_t retcode;
    sai_fdb_api_t *saiFdbApi;
    retcode =  sai_api_query(SAI_API_FDB, (void**) &saiFdbApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "API Query Fail in VendorPurgeMacEntriesPerPort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Find port sai.
    //
    sai_object_id_t portSai;
    if (!esalFindBridgePortSaiFromPortId(port, &portSai)) {
        std::cout << "port_table_find_sai fail:" << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Create Attribute list.
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_FDB_FLUSH_ATTR_BRIDGE_PORT_ID;
    attr.value.oid = portSai;
    attributes.push_back(attr); 

    retcode = saiFdbApi->flush_fdb_entries(
        esalSwitchId, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "Flush FDB Fail in VendorPurgeMacEntriesPerPort\n"));
        std::cout << "Fail flush_fdb_entries: " << esalSaiError(retcode) << "\n";
    }
#endif

    return rc;
}

int VendorPurgeMacEntries(void) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
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
        std::cout << "Fail flush_fdb_entries: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    return rc;
}

int VendorGetMacTbl(uint16_t port, uint16_t *numMacs, unsigned char *macs) {
    std::cout << __PRETTY_FUNCTION__ << port << std::endl;
    int rc  = ESAL_RC_OK;
    int maxMacs = (*numMacs > 256) ? *numMacs : 256;
    
    *numMacs = 0; 
    for(int i = 0; i < fdbTableSize; i++) {
        auto &fdb = fdbTable[i];
        if (fdb.egressPort == port) {
            memcpy(
               macs+((*numMacs)*sizeof(sai_mac_t)), fdb.macAddr, sizeof(sai_mac_t));
 
            (*numMacs)++; 
            if (*numMacs >= maxMacs) break; 
        }
    }

    return rc;
}


}


