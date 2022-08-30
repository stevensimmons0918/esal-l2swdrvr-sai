/**
 * @file      esalSaiHost.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. SAI HOST
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"

#include <iostream>
#include <arpa/inet.h> 

#include <string>
#include <cinttypes>
#include "mutex"
#include "vector"
#include "string"

#ifndef UTS
#include "sai/sai.h"
#include "sai/saihostif.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif
#endif

#include "esal-vendor-api/esal_vendor_api.h"
#include "lib/swerr.h"
#ifndef UTS 
#ifndef LARCH_ENVIRON
#include "pf_proto/esal_apps.pb.h"
#endif
#endif



extern "C" {


// APPROACH TO SEMAPHORE:
//   There are multiple threads for configuring the filter table,
//   as well as another thread for Packet Rx.  The following algorithm is
//   used for protecting the filter table during multi-tasking.  The following
//   assumptions are built into the mutex design:
// 
//        - The entire table must be iterated, looking for matches on either 
//          VLAN ID SAI Object, PORT SAI Object, or combination of both.
//        - Filter Tables only grow.
//        - A typical filter table is ~8 filters, maximum is 32.
//        - Cannot use operating system primitives for Packet Rx/Tx.
//        - Can use operating system primitives for update
// 
//   Therefore, there will be a "C" style arrays with entry containing two
//   fields: PORT SAI Object and VLAN ID. More importantly, there is a current
//   table size attribute.  All updates are made in the shadow area, above
//   the current table size.   Then, the current table size is incremented.
//   A sempahore protects the following sequence of code
// 
//       o Instantiation of SAI Object.
//       o Writing to the C Array
//       o Bumping the C Array Size
// 


const int MAC_SIZE = sizeof(sai_mac_t);
#ifdef LARCH_ENVIRON

struct EsalL2Filter {

    std::string name = "FOO";

    std::string mc = "00:de:ad:be:ef:00";

    std::string &mac() { return mc; };

    std::string mcMask = "ff:ff:ff:80:00:00";

    std::string &macmask() { return mcMask; };

    std::string &filtername() { return name; };

    bool has_mac() { return true; };

    bool has_macmask() { return true; };

    bool has_vlan() { return true; };

    bool has_vlanmask() { return true; };

    uint16_t vlan() { return 33; };

    uint16_t vlanmask() { return 0xff; } ;

};

 

struct EsalAppsRegMessage {

    EsalL2Filter fltr;

    EsalL2Filter &filter() { return fltr; };

};

 
#endif
struct FilterEntry{
    std::string filterName; 
    EsalL2Filter filter; 
    unsigned char mac[MAC_SIZE];
    unsigned char macMask[MAC_SIZE];
    bool pendingDelete; 
};



const int MAX_FILTER_TABLE_SIZE = 32;
static FilterEntry filterTable[MAX_FILTER_TABLE_SIZE];
static int filterTableSize = 0;
static std::mutex filterTableMutex;
static VendorRxCallback_fp_t rcvrCb;
static void *rcvrCbId;
sai_object_id_t hostInterface;

static void convertMacStringToAddr(std::string &macString, unsigned char *macAddr) {

    int macIdx = 0;
    memset(macAddr, 0, MAC_SIZE);
    for(auto ch : macString) {
        unsigned char num = 0; 
        switch (ch) {
            case ':':
                macIdx++;
                break;
            case 'A':
            case 'a':
                num = 10;
                break;
            case 'B':
            case 'b':
                num = 11;
                break;
            case 'C':
            case 'c':
                num = 12;
                break;
            case 'D':
            case 'd':
                num = 13;
                break;
            case 'E':
            case 'e':
                num = 14;
                break;
            case 'F':
            case 'f':
                num = 15;
                break;
            default:
                if (ch >= '0' && ch <= '9') {
                   num = ch - '0';
                }
        }
        if (macIdx <  MAC_SIZE) {
            macAddr[macIdx] = (macAddr[macIdx]*16) + num;
        }
    }
}

static bool searchFilterTable(
    uint16_t port, int &idx, const void *buffer, sai_size_t bufferSz){

    const char *bufPtr = (const char*) buffer;
    unsigned char macAddr[MAC_SIZE];
 
    // Check to see that the buffer is bigger than minimum size
    //
    if (bufferSz > ((2*MAC_SIZE)+2+2)) {
        return false;
    }

    // Assuming packet starts at DST MAC.
    //     DSTMAC[6] SRCMAC[6] ETYPE[2] VLANID[2]
    //
    memcpy(macAddr, buffer, MAC_SIZE);
 
    // Get VLAN. 
    //
    uint16_t vlan = 0;
    char *vlanPtr = (char*) &vlan; 
    if ((bufPtr[12] == 0x81) && (bufPtr[13] == 0x00)) {
        vlanPtr[0] = bufPtr[12];
        vlanPtr[1] = bufPtr[13];
        vlan = ntohs(vlan);
        vlan &= 0xfff; 
    }

    // Search table.
    //
    for(int i = 0; i < filterTableSize; i++) {
        bool matching = true; 
        EsalL2Filter &fltr = filterTable[i].filter;
  
        // Ignore if marked pending delete. 
        //
        if (filterTable[i].pendingDelete) continue; 

        // VLAN present.
        // 
        if (fltr.has_vlan()) {
            uint16_t vlanMask =  0xfff;
            if (fltr.has_vlanmask()) {
                vlanMask = fltr.vlanmask();
            }

            if ((vlan & vlanMask) != (fltr.vlan() & vlanMask)) {
                matching = false;
            }
        }
#ifndef UTS
          
        // Check MAC. 
        //
        if (fltr.has_mac() && matching) {
            unsigned char macMatch[MAC_SIZE];
            memcpy(macMatch, filterTable[i].mac, MAC_SIZE);
            unsigned char macMask[MAC_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
            if (fltr.has_macmask()) {
                memcpy(macMask, filterTable[i].macMask, MAC_SIZE);
            }
            for (int i = 0; i < MAC_SIZE; i++) {
                if ((macMatch[i] & macMask[i]) != (macAddr[i] & macMask[i])) {
                    matching = false; 
                    break; 
                }
            }
        }
#ifndef LARCH_ENVIRON
        // Check Raw Data
        // 
        if (fltr.rawdata_size() && matching) {
            for (auto i = 0; i < fltr.rawdata_size(); i++){
                const char *bufPtr = (const char*) buffer;
                auto offset = fltr.rawdata(i).offset(); 
                if ((offset+sizeof(long))  > bufferSz) {
                    matching = false; 
                    break; 
                }
                auto data = fltr.rawdata(i).data(); 
                auto mask = fltr.rawdata(i).mask(); 
                const char *pktPtr = bufPtr+offset;

                // Coerce to longword alignment. Assuming mask handles bufsize that
                // is not multiple of 4 length. 
                //
                unsigned long pkt =
                    (pktPtr[offset] << 24) | (pktPtr[offset+1] << 16) | 
                    (pktPtr[offset+2] << 8) | (pktPtr[offset+3]);

                if ((pkt & mask) != (data & mask)) {
                    matching = false; 
                    break; 
                }
            }
        }
#endif
#endif
        // FIXME ... expecting to compare against interface name but
        // no knowledge of interface name. ClientIntf
        //
        if (matching) {
            idx = i;  
            return true; 
        }
    }

    return false;

}

bool esalHandleSaiHostRxPacket(
    const void *buffer, sai_size_t bufferSz, uint32_t attrCnt, const sai_attribute_t *attrList) {


    // Check to see if callback is registered yet. 
    //
    if (!rcvrCb) {
        return false; 
    }

    // Find the incoming port. 
    //
    sai_object_id_t portSai = SAI_NULL_OBJECT_ID;
#ifdef UTS
    portSai = 155;
#else
    for(uint32_t i = 0; i < attrCnt; i++) {
        if (attrList[i].id == SAI_HOSTIF_PACKET_ATTR_INGRESS_PORT) {
            portSai = attrList[i].value.oid; 
        }
    }
#endif

    // Check to see portSai is found.
    //
    if (portSai == SAI_NULL_OBJECT_ID) {
        return false;
    }

    // Convert from SAI to Port Id. 
    //
    uint16_t portId = 0;
    if (!esalPortTableFindId(portSai, &portId)){
        return false;
    }

    // Search the filter table for this port. 
    //
    const char *fname = 0; 
    int idx; 
    if (searchFilterTable(portId, idx, buffer, bufferSz)) {
        fname = filterTable[idx].filterName.c_str(); 
    } 

    // Handle callback.
    //
    return rcvrCb(rcvrCbId, fname, portId, (uint16_t) bufferSz, (void*) buffer); 

}


int VendorRegisterRxCb(VendorRxCallback_fp_t cb, void *cbId) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    rcvrCb = cb;
    rcvrCbId = cbId; 
    return ESAL_RC_OK;
}

int VendorAddPacketFilter(const char *buf, uint16_t length) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::unique_lock<std::mutex> lock(filterTableMutex);

    // Make sure filter table is not exhausted.
    //
    if (filterTableSize >= MAX_FILTER_TABLE_SIZE) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "table exhausted in VendorAddPacketFilter\n"));
        std::cout << "Packet Filter Table Exhausted: " << filterTableSize << "\n";
        return ESAL_RC_FAIL; 
    }
    //  Get App registration message.  It must be set pkt filter message. 
    //
    EsalAppsRegMessage regMsg;
#ifndef LARCH_ENVIRON
#ifndef UTS
    regMsg.ParseFromArray(buf, length); 
    if ((regMsg.type() == EsalAppsRequestType::SET_PKT_FILTER) &&
        (regMsg.has_filter())){
#endif 
#endif
        // Get the filter.
        //
        EsalL2Filter filter = regMsg.filter();
        if (filter.filtername().empty()) {
            return ESAL_RC_OK; 
        }
        // Check to see if insert the same filter name, key into the filter 
        // table. 
        //
        std::string filterName = filter.filtername();
        for (int i = 0; i < filterTableSize; i++) {
            if (filterTable[i].filterName == filterName) {
                return ESAL_RC_OK;
            }
        }
        
        // Add in the shadow.
        // 
        FilterEntry *newEntry = filterTable+filterTableSize; 
        newEntry->pendingDelete = false;
        newEntry->filterName = filterName;
        newEntry->filter = filter;
        
        // Convert MAC Address to binary representation for optimization.
        //
        if (filter.has_mac()) {
            auto macString = filter.mac(); 
            convertMacStringToAddr(macString, newEntry->mac);
        }
        if (filter.has_macmask()) {
            auto macMaskString = filter.macmask(); 
            convertMacStringToAddr(macMaskString, newEntry->macMask);
        }
        filterTableSize++;
#ifndef LARCH_ENVIRON
#ifndef UTS
    }
#endif
#endif
    return ESAL_RC_OK;

}

int VendorDeletePacketFilter(const char *filterName) {

    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::unique_lock<std::mutex> lock(filterTableMutex);

    // Look for match first. 
    //
    int idx = 0; 
    for (idx = 0; idx < filterTableSize; idx++) {
        if (filterTable[idx].filterName == filterName) {
            filterTable[idx].pendingDelete = true;
            break;
        }
    }

    // Check to see if match is found
    //
    if (idx == filterTableSize) {
        return ESAL_RC_OK;
    }

    // Update in the shadow.
    //
    filterTable[idx] = filterTable[filterTableSize-1];
    filterTable[idx].pendingDelete = false;
    filterTableSize--;
    
    return ESAL_RC_OK;

}

int VendorSendPacket(uint16_t portId, uint16_t length, const void *buf) {
    std::cout << __PRETTY_FUNCTION__ << " " << portId << std::endl;
#ifndef UTS
    sai_status_t retcode = SAI_STATUS_SUCCESS;
    sai_hostif_api_t *sai_hostif_api;

    retcode =  sai_api_query(SAI_API_HOSTIF, (void**) &sai_hostif_api);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query in VendorSendPacket\n"));
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    std::vector<sai_attribute_t> attrList;

    attr.id = SAI_HOSTIF_PACKET_ATTR_HOSTIF_TX_TYPE;
    attr.value.s32 = SAI_HOSTIF_TX_TYPE_PIPELINE_BYPASS;
    attrList.push_back(attr);

    // Look up port to get SAI object.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(portId, &portSai)){
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalPortTableFindSai in VendorSendPacket\n"));
        return ESAL_RC_FAIL; 
    }

    attr.id = SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG;
    attr.value.oid = portSai;
    attrList.push_back(attr);

    // Send the packet.
    //
    retcode = sai_hostif_api->send_hostif_packet(
        hostInterface, length, buf, attrList.size(), attrList.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "send_hostif_packet in VendorSendPacket\n"));
        return ESAL_RC_FAIL;
    }

#endif
    return ESAL_RC_OK;
}

int esalCreateSaiHost(uint16_t portId, const char *name) {

#ifndef UTS
    sai_status_t retcode = SAI_STATUS_SUCCESS;
    sai_hostif_api_t *sai_hostif_api;

   
    retcode =  sai_api_query(SAI_API_HOSTIF, (void**) &sai_hostif_api);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalCreateSaiHost\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    std::vector<sai_attribute_t> attrList;

    // Create a host interface
    //  
    attr.id = SAI_HOSTIF_ATTR_TYPE;
    attr.value.s32 = SAI_HOSTIF_TYPE_NETDEV;
    attrList.push_back(attr);
    
    // Set the respective port. 
    //
    attr.id = SAI_HOSTIF_ATTR_OBJ_ID;
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(portId, &portSai)){
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalPortTableFindSai fail in esalCreateSaiHost\n"));
        return ESAL_RC_FAIL;
    }
    attr.value.oid = portSai; 
    attrList.push_back(attr);
    
    attr.id = SAI_HOSTIF_ATTR_NAME;
    memcpy((char *)&attr.value.chardata, name, SAI_HOSTIF_NAME_SIZE);
    attrList.push_back(attr);
    
    // Current implementation of MRVL SAI does not support these attributes
    // attr.id = SAI_HOSTIF_ATTR_OPER_STATUS;
    // attr.value.booldata = true;
    // attrList.push_back(attr);
    
    // attr.id = SAI_HOSTIF_ATTR_VLAN_TAG;
    // attr.value.u32 = SAI_HOSTIF_VLAN_TAG_ORIGINAL;
    // attrList.push_back(attr);
    
    
    retcode = sai_hostif_api->create_hostif(
        &hostInterface, esalSwitchId, attrList.size(), attrList.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_hostif fail in esalCreateSaiHost\n"));
        std::cout << "create_hostif failed: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif
   
    return ESAL_RC_OK; 
}

int esalRemoveSaiHost(void) {

#ifndef UTS
    sai_status_t retcode = SAI_STATUS_SUCCESS;
    sai_hostif_api_t *sai_hostif_api;

    // Determina the API 
    //
    retcode =  sai_api_query(SAI_API_HOSTIF, (void**) &sai_hostif_api);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalRemoveSaiHost\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }

    // Remove host interface. 
    //
    retcode = sai_hostif_api->remove_hostif(hostInterface);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalRemoveSaiHost\n"));
        std::cout << "create_hostif failed: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL;
    }
#endif

    // Empty the filter table. 
    //
    filterTableSize = 0;
    return ESAL_RC_OK; 
}

}
