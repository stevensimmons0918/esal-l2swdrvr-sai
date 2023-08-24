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
#include "headers/esalSaiUtils.h"
#include <iostream>
#include <arpa/inet.h> 
#include <string>
#include <cinttypes>
#include "mutex"
#include "vector"
#include "string"

#include "sai/sai.h"
#include "sai/saihostif.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif

#include "esal_vendor_api/esal_vendor_api.h"
#ifndef UTS 
#ifndef LARCH_ENVIRON
#include "pf_proto/esal_apps.pb.h"
#endif
#endif

extern "C" {

// APPROACH TO SEMAPHORE:
// There are multiple threads for configuring the filter table,
// as well as another thread for Packet Rx.  The following algorithm is
// used for protecting the filter table during multi-tasking.  The following
// assumptions are built into the mutex design:
// 
//        - The entire table must be iterated, looking for matches on either 
//          VLAN ID SAI Object, PORT SAI Object, or combination of both.
//        - Filter Tables only grow.
//        - A typical filter table is ~8 filters, maximum is 32.
//        - Cannot use operating system primitives for Packet Rx/Tx.
//        - Can use operating system primitives for update
// 
// Therefore, there will be a "C" style arrays with entry containing two
// fields: PORT SAI Object and VLAN ID. More importantly, there is a current
// table size attribute.  All updates are made in the shadow area, above
// the current table size.   Then, the current table size is incremented.
// A sempahore protects the following sequence of code
// 
//       o Instantiation of SAI Object.
//       o Writing to the C Array
//       o Bumping the C Array Size
// 

const int MAC_SIZE = sizeof(sai_mac_t);

#define VLAN_ETHERTYPE_OFFSET 12
#define VLAN_ID_OFFSET        14
#define ETHERTYPE_OFFSET      16
#define IPV4_PROTO_OFFSET     28
#define UDP_DST_PORT_OFFSET   40
#ifdef LARCH_ENVIRON

class  RawData {
    int offset_field[5] = {VLAN_ETHERTYPE_OFFSET, VLAN_ID_OFFSET, ETHERTYPE_OFFSET, IPV4_PROTO_OFFSET, UDP_DST_PORT_OFFSET}; 
    int data_field[5] = {0x8100, 2003, 0x800, 17, 67};
    int mask_field[5] = {0xffff, 0xfff, 0xffff, 0xffff, 0xffff}; 
    int index; 

    public:
    int offset() { return offset_field[index];};
    int data() { return data_field[index];};
    int mask() { return mask_field[index];};
    RawData& operator() (int i)
    {
        index = i;
        return *this;
    }
};

class EsalL2Filter {
    public:

    std::string name = "FOO";

    std::string mc = "01:80:C2:00:00:FF";

    std::string &mac() { return mc; };

    std::string mcMask = "ff:ff:ff:ff:ff:ff";

    std::string &macmask() { return mcMask; };

    std::string &filtername() { return name; };

    bool has_mac() { return false; };

    bool has_macmask() { return false; };

    bool has_vlan() { return false; };

    bool has_vlanmask() { return false; };

    uint16_t vlan() { return 100; };

    uint16_t vlanmask() { return 0xff; } ;

    int vendorport_size() { return 1; };
    uint32_t vendorport(int) { return 28; };
    
    RawData rawData;

    int rawdata_size() { return 5;} //rawdata_array.size(); };
    RawData rawdata(int index) { return rawData(index); };
};
// filter.rawdata(i).offset();

#endif

struct FilterEntry{
    std::string filterName; 
    EsalL2Filter filter; 
    unsigned char mac[MAC_SIZE];
    unsigned char macMask[MAC_SIZE];
    bool pendingDelete; 
    sai_object_id_t aclEntryOid;
    sai_object_id_t aclEntryV6Oid;

};

const int MAX_FILTER_TABLE_SIZE = 32;
static FilterEntry filterTable[MAX_FILTER_TABLE_SIZE];
static int filterTableSize = 0;
static std::mutex filterTableMutex;
static VendorRxCallback_fp_t rcvrCb;
static void *rcvrCbId;
sai_object_id_t hostInterface;



static void convertMacStringToAddr(std::string &macString,
                                   unsigned char *macAddr) {
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
    uint32_t lPort, int &idx, const void *buffer, sai_size_t bufferSz) {
    const unsigned char *bufPtr = (const unsigned char*) buffer;
    unsigned char macAddr[MAC_SIZE];
 
    // Check to see that the buffer is bigger than minimum size
    if (bufferSz < ((2*MAC_SIZE)+2+2)) {
        return false;
    }

    // Buffer is packet with the following format:
    //    
    //    DST MAC: 6 bytes
    //    SRC MAC: 6 bytes
    //    VLAN Type: 2 bytes ... must be 0x8100
    //    VLAN ID: 2 bytes
    //    Ether Type for packet : 2 bytes

    // Assuming packet starts at DST MAC.
    //     DSTMAC[6] SRCMAC[6] ETYPE[2] VLANID[2]
   
    memcpy(macAddr, buffer, MAC_SIZE);
 
    // Get VLAN. 
    uint16_t vlan = 0;
    unsigned char *vlanPtr = (unsigned char*) &vlan; 
    if ((bufPtr[12] == 0x81) && (bufPtr[13] == 0x00)) {
        vlanPtr[0] = bufPtr[14];
        vlanPtr[1] = bufPtr[15];
        vlan = ntohs(vlan);
        vlan &= 0xfff; 
    }

    // Search table.
    for(int i = 0; i < filterTableSize; i++) {
        bool matching = true; 
        EsalL2Filter &fltr = filterTable[i].filter;
  
        // Ignore if marked pending delete. 
        if (filterTable[i].pendingDelete) continue; 

        // Check to see if logical port matches.
        auto vpsize = fltr.vendorport_size(); 
        if (vpsize) {
            matching = false;
            for(int vpidx = 0; vpidx < vpsize; vpidx++) {
                if (fltr.vendorport(vpidx) == lPort) {
                    matching = true;
                    break; 
                }
            }
            if (!matching) continue; 
        }

        // VLAN present.
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
            unsigned char macMask[MAC_SIZE] = {0xff, 0xff, 0xff,
                                               0xff, 0xff, 0xff};
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
        if (fltr.rawdata_size() && matching) {
            for (auto i = 0; i < fltr.rawdata_size(); i++){
                unsigned const char *bufPtr = (unsigned const char*) buffer;
                auto offset = fltr.rawdata(i).offset(); 
                if ((offset*sizeof(long))  > bufferSz) {
                    matching = false; 
                    break; 
                }
                auto data = fltr.rawdata(i).data(); 
                auto mask = fltr.rawdata(i).mask(); 
                unsigned const char *pktPtr = bufPtr+offset;

                // Coerce to longword alignment.
                // Assuming mask handles bufsize that
                // is not multiple of 4 length. 
                unsigned long pkt =
                    (pktPtr[offset] << 24) | (pktPtr[offset+1] << 16) | 
                    (pktPtr[offset+2] << 8) | (pktPtr[offset+3]);

                pkt = ntohl(pkt);
                
                // Compare the packet contents masked with the raw data masked. 
                //  
                if ((pkt & mask) != (data & mask)) {
                    matching = false;
                    break;
                }
            }
        }
#endif
#endif
        if (matching) {
            idx = i;
            return true;
        }
    }

    return false;
}

bool esalHandleSaiHostRxPacket(const void *buffer,
                               sai_size_t bufferSz, uint32_t attrCnt,
                               const sai_attribute_t *attrList) {

    // Check to see if callback is registered yet. 
    if (!rcvrCb) {
        return false;
    } 

    // Find the incoming port.
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
    if (portSai == SAI_NULL_OBJECT_ID) {
        return false;
    }

    // Convert from SAI to Port Id.
    uint16_t portId = 0;
    if (!esalPortTableFindId(portSai, &portId)) {
        return false;
    }

    // Convert to logical port. 
    uint32_t lPort;
    if (!saiUtils.GetLogicalPort(0, portId, &lPort)) {
        return false;
    }

    // Search the filter table for this port. 
    const char *fname = 0;
    int idx;
    if (searchFilterTable(lPort, idx, buffer, bufferSz)) {
        fname = filterTable[idx].filterName.c_str();
    } else {
        return false;
    }

    return rcvrCb(rcvrCbId, fname, lPort,
                  (uint16_t) bufferSz, (void*) buffer);
}


int VendorRegisterRxCb(VendorRxCallback_fp_t cb, void *cbId) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    rcvrCb = cb;
    rcvrCbId = cbId; 
    return ESAL_RC_OK;
}

int VendorAddPacketFilter(const char *buf, uint16_t length) {
    std::cout << "VendorAddPacketFilter:" << std::endl;
    if (!useSaiFlag){
        return false;
    }

    uint16_t vlan = 0;
    bool matching = false;
    uint32_t dev;
    uint32_t lPort;
    uint32_t pPort;
    std::vector<sai_object_id_t> port_list;
    uint16_t VlanTagEtherType = 0;
    uint16_t VlanId = 0;
    uint16_t EtherType = 0;
    uint8_t IPv4Proto = 0;
    uint16_t UdpDstPort = 0;

    std::unique_lock<std::mutex> lock(filterTableMutex);

    // Make sure filter table is not exhausted.
    if (filterTableSize >= MAX_FILTER_TABLE_SIZE) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "table exhausted in VendorAddPacketFilter\n"));
        std::cout << "Packet Filter Table Exhausted: " << filterTableSize
                  << std::endl;
        return ESAL_RC_FAIL; 
    }

    //  Get App registration message.  It must be set pkt filter message. 
    EsalL2Filter filter;
#ifndef LARCH_ENVIRON
#ifndef UTS
    filter.ParseFromArray(buf, length);
#endif
#endif
    if (filter.filtername().empty()) {
        return ESAL_RC_FAIL; 
    }
    // Check to see if insert the same filter name, key into the filter 
    // table. 
    std::string filterName = filter.filtername();
    for (int i = 0; i < filterTableSize; i++) {
        if (filterTable[i].filterName == filterName) {
            return ESAL_RC_OK;
        }
    }

    // Add in the shadow.
    FilterEntry *newEntry = filterTable+filterTableSize;
    newEntry->pendingDelete = false;
    newEntry->filterName = filterName;
    newEntry->filter = filter;

    // Entry
    //
    aclEntryAttributes aclEntryAttr;
    memset(&aclEntryAttr, 0, sizeof(aclEntryAttributes));

    //Convert MAC Address to binary representation for optimization.
    if (filter.has_mac()) {
        auto macString = filter.mac();
        convertMacStringToAddr(macString, newEntry->mac);
        aclEntryAttr.field_dst_mac.enable = true;
        memcpy(aclEntryAttr.field_dst_mac.data.mac, newEntry->mac, sizeof(sai_mac_t));
    }
    if (filter.has_macmask()) {
        auto macMaskString = filter.macmask();
        convertMacStringToAddr(macMaskString, newEntry->macMask);
        memcpy(aclEntryAttr.field_dst_mac.mask.mac, newEntry->macMask, sizeof(sai_mac_t));
    }

    if (filter.has_vlan()) {
        uint16_t vlanMask = 0xfff;
        if (filter.has_vlanmask()) {
            vlanMask = filter.vlanmask();
        }

        vlan = filter.vlan();

        if ((vlan & vlanMask) != (filter.vlan() & vlanMask)) {
            matching = false;
        }

        aclEntryAttr.field_outer_vlan_id.enable = true;
        aclEntryAttr.field_outer_vlan_id.data.u16 = vlan;
        aclEntryAttr.field_outer_vlan_id.mask.u16 = vlanMask;
    }

#ifndef UTS
    if (filter.rawdata_size()) {
        for (auto i = 0; i < filter.rawdata_size(); i++) {
            auto offset = filter.rawdata(i).offset();
            auto data = filter.rawdata(i).data();
            auto mask = filter.rawdata(i).mask(); 
            switch (offset) {
                case VLAN_ETHERTYPE_OFFSET: 
                    VlanTagEtherType = data;
                break;
                
                case VLAN_ID_OFFSET: 
                    aclEntryAttr.field_outer_vlan_id.data.u16 = data;
                    aclEntryAttr.field_outer_vlan_id.mask.u16 = mask;
                    VlanId = data;
                break;
                
                case ETHERTYPE_OFFSET: 
                    aclEntryAttr.field_ether_type.data.u16 = data;
                    aclEntryAttr.field_ether_type.mask.u16 = mask;
                    EtherType = data;
                break;
                
                case IPV4_PROTO_OFFSET: 
                    aclEntryAttr.field_ip_protocol.data.u8 = data;
                    aclEntryAttr.field_ip_protocol.mask.u8 = mask;
                    IPv4Proto = data;
                break;
                
                case UDP_DST_PORT_OFFSET: 
                    aclEntryAttr.field_l4_dst_port.data.u16 = data;
                    aclEntryAttr.field_l4_dst_port.mask.u16 = mask;
                    UdpDstPort = data;
                break;

                default:
                    break;
            }
        }

        // Check if it`s DHCP filter

        if (VlanTagEtherType == 0x8100 &&
            VlanId == 2003 &&
            EtherType == 0x800 &&
            IPv4Proto == 17 &&
            (UdpDstPort == 67 || UdpDstPort == 68)) {
                aclEntryAttr.field_outer_vlan_id.enable = true;
                aclEntryAttr.field_ether_type.enable = true;
                aclEntryAttr.field_ip_protocol.enable = true;
                aclEntryAttr.field_l4_dst_port.enable = true;
        }
    }
#else
    (void) VlanTagEtherType;
    (void) VlanId;
    (void) EtherType;
    (void) IPv4Proto;
    (void) UdpDstPort;
#endif

    // Check to see if logical port matches.
    auto vpsize = filter.vendorport_size();
    if (vpsize) {
        
        
        // Find the sai port.
        sai_object_id_t portSai;

        for (int vpidx = 0; vpidx < vpsize; vpidx++) {
            lPort = filter.vendorport(vpidx);
            if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
                std::string err = "VendorAddPacketFilter, failed to get pPort, " 
                                "lPort=" + lPort;
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, err.c_str()));
                return ESAL_RC_FAIL;
            }
                    
            if (!esalPortTableFindSai(pPort, &portSai)) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalPortTableFindSai fail " \
                                    "in VendorEnablePort\n"));
                std::cout << "esalPortTableFindSai fail pPort: " << pPort << std::endl;
                return ESAL_RC_FAIL;
            }
            port_list.push_back(portSai);
        }
        sai_acl_field_data_t match_in_ports;
        match_in_ports.enable = true;
        match_in_ports.data.objlist.count = (uint32_t)port_list.size();
        match_in_ports.data.objlist.list = port_list.data();
        aclEntryAttr.field_in_ports = match_in_ports;
    }

    aclEntryAttr.table_id = packetFilterAclTableOid;

    aclEntryAttr.action_packet_action.enable = true;
    aclEntryAttr.action_packet_action.parameter.s32 = SAI_PACKET_ACTION_TRAP;

    sai_object_id_t aclEntryOid;
    if (!esalCreateAclEntry(aclEntryAttr, aclEntryOid) && matching) {
        // std::string err = "VendorAddPacketFilter, failed to create Acl entry in Hw, " 
        //                     "lPort=" + lPort;
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "create acl entry fail"));
            return ESAL_RC_FAIL;
    }

    aclEntryAttr.table_id = packetFilterIpV6AclTableOid;

    sai_object_id_t aclEntryV6Oid;
    if(!esalCreateAclEntry(aclEntryAttr, aclEntryV6Oid) && matching) {
        // std::string err = "VendorAddPacketFilter, failed to create Acl entry in Hw, " 
        //                     "lPort=" + lPort;
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "failure of createAclEntry"));
            return ESAL_RC_FAIL;
    }
    // Save oids for acl in filter table
    newEntry->aclEntryOid = aclEntryOid;
    newEntry->aclEntryV6Oid = aclEntryV6Oid;
    
    filterTableSize++;

    return ESAL_RC_OK;
}

int VendorDeletePacketFilter(const char *filterName) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    std::unique_lock<std::mutex> lock(filterTableMutex);

    // Look for match first. 
    int idx = 0;
    for (idx = 0; idx < filterTableSize; idx++) {
        if (filterTable[idx].filterName == filterName) {
            filterTable[idx].pendingDelete = true;
            break;
        }
    }

    // Check to see if match is found
    if (idx == filterTableSize) {
        return ESAL_RC_OK;
    }

    if (!esalRemoveAclEntry(filterTable[idx].aclEntryOid)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalRemoveAclEntry failed \n"));
        std::cout << "esalRemoveAclEntry fail " << std::endl;
        return ESAL_RC_FAIL;
    }

    if (!esalRemoveAclEntry(filterTable[idx].aclEntryV6Oid)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalRemoveAclEntry failed\n"));
        std::cout << "esalRemoveAclEntry failed" << std::endl;
        return ESAL_RC_FAIL;
    }

    // Update in the shadow.
    filterTable[idx] = filterTable[filterTableSize-1];
    filterTable[idx].pendingDelete = false;
    filterTableSize--;

    return ESAL_RC_OK;
}

int VendorSendPacket(uint16_t lPort, uint16_t length, const void *buf) {

#ifndef UTS

    sai_status_t retcode = SAI_STATUS_SUCCESS;
    sai_hostif_api_t *sai_hostif_api;
    uint32_t dev;
    uint32_t pPort;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::string err = "VendorSendPacket, failed to get pPort, " \
                          "lPort=" + lPort;
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, err.c_str()));
        return ESAL_RC_FAIL;
    }

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
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)){
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalPortTableFindSai in VendorSendPacket\n"));
        return ESAL_RC_FAIL; 
    }

    attr.id = SAI_HOSTIF_PACKET_ATTR_EGRESS_PORT_OR_LAG;
    attr.value.oid = portSai;
    attrList.push_back(attr);

    // Send the packet.
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
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }

    sai_attribute_t attr;
    std::vector<sai_attribute_t> attrList;

    // Create a host interface
    attr.id = SAI_HOSTIF_ATTR_TYPE;
    attr.value.s32 = SAI_HOSTIF_TYPE_NETDEV;
    attrList.push_back(attr);

    // Set the respective port.
    attr.id = SAI_HOSTIF_ATTR_OBJ_ID;
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(portId, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "esalPortTableFindSai fail " \
                              "in esalCreateSaiHost\n"));
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
        std::cout << "create_hostif failed: " << esalSaiError(retcode)
                  << std::endl;
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
    retcode =  sai_api_query(SAI_API_HOSTIF, (void**) &sai_hostif_api);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalRemoveSaiHost\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }

    // Remove host interface.
    retcode = sai_hostif_api->remove_hostif(hostInterface);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query fail in esalRemoveSaiHost\n"));
        std::cout << "create_hostif failed: " << esalSaiError(retcode)
                  << std::endl;
        return ESAL_RC_FAIL;
    }
#endif

    // Empty the filter table.
    filterTableSize = 0;
    return ESAL_RC_OK;
}

}
