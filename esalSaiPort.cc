/**
 * @file      esalSaiPort.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. Supports the SAIPORT object
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#ifdef HAVE_MRVL
#include "headers/esalCpssDefs.h"
#endif
#include <iostream>

#include <string>
#include <cinttypes>
#include <mutex>
#include <vector>

#include "esal_vendor_api/esal_vendor_api.h"

#ifndef UTS
#include "sai/sai.h"
#include "sai/saiport.h"
#ifndef LARCH_ENVIRON
#include "sfp_vendor_api/sfp_vendor_api.h"
#endif
#endif

extern "C" {

// APPROACH TO SEMAPHORE: 
//   There are multiple threads for configuring the local port table,
//   as well as another thread for Packet Rx.  The following algorithm is 
//   used for protecting PORT ID <-map-> PORT SAI Object.  The following 
//   assumptions are built into the mutex design:
//   
//         - Both Port ID and SAI Object must be keys for look-up. 
//         - Port Tables only grow. 
//         - A typical port configuration is ~32 ports, maximum is 512. 
//         - Cannot use operating system primitives for Packet Rx/Tx.
//         - Can use operating system primitives for update
//
//  Therefore, there will be a "C" style arrays with entry containing two
//  fields: PORT ID and SAI Object. More importantly, there is a current
//  table size attribute.  All updates are made in the shadow area, above
//  the current table size.   Then, the current table size is incremented.
//  A sempahore protects the following sequence of code
//  
//  	o Instantiation of SAI Object.  
//  	o Writing to the C Array
//  	o Bumping the C Array Size
//




struct SaiPortEntry{
    uint16_t portId;
    sai_object_id_t portSai;
};

const int MAX_PORT_TABLE_SIZE = 512;
SaiPortEntry portTable[MAX_PORT_TABLE_SIZE];
int portTableSize = 0;
std::mutex portTableMutex; 

bool esalPortTableFindId(sai_object_id_t portSai, uint16_t* portId) {
   
    // Search array for match.
    //
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portSai == portSai) {
            *portId = portTable[i].portId;
            return true;
        }
    }
    *portId = 0;
    return false; 

}

bool esalPortTableFindSai(uint16_t portId, sai_object_id_t *portSai) {
   
    // Search array for match.
    //
    for(auto i = 0; i < portTableSize; i++) {
        if (portTable[i].portId == portId) {
            *portSai = portTable[i].portSai;
            return true;
        }
    }
    *portSai = SAI_NULL_OBJECT_ID;
    return false; 

}

bool esalPortTableGetSaiByIdx(uint16_t idx, sai_object_id_t *portSai) {
    
    // Return port oid by idx if exist
    //
    if (portTable[idx].portSai != SAI_NULL_OBJECT_ID) {
        *portSai = portTable[idx].portSai;
        return true;
    }
    *portSai = SAI_NULL_OBJECT_ID;
    return false;
}

bool esalPortTableAddEntry(uint16_t portId, sai_object_id_t *portSai){

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(portTableMutex);

    // Check for max exceeded. 
    // 
    if (portTableSize >= MAX_PORT_TABLE_SIZE) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "table full in esalPortTableAddEntry\n"));
        std::cout << "esalPortTableAddEntry: max table exceed: " << portId << "\n";
        return false;
    }

#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalPortTableAddEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return false; 
    }

    if (portSai == SAI_NULL_OBJECT_ID) {
        // Instantiate a new port sai. Mandatory attributes 
        // Adding port speed now as 1G default, but assuming it will change.
        //
        std::vector<sai_attribute_t> attributes;
        sai_attribute_t attr;

        attr.id = SAI_PORT_ATTR_HW_LANE_LIST;
        std::vector<sai_uint32_t> hwLanes;
        hwLanes.push_back(portId); 

        attr.value.u32list.list = const_cast<sai_uint32_t*>(hwLanes.data());;
        attr.value.u32list.count = static_cast<sai_uint32_t>(hwLanes.size());
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_SPEED;
        attr.value.u32 = 1000;
        attributes.push_back(attr);

        attr.id = SAI_PORT_ATTR_ADMIN_STATE;
        attr.value.booldata = false;
        attributes.push_back(attr);

        // Create a port.
        //
        retcode = saiPortApi->create_port(
            portSai, esalSwitchId, attributes.size(), attributes.data());
        if (retcode)
        {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "create_port Fail in esalPortTableAddEntry\n"));
            std::cout << "create_port fail: " << esalSaiError(retcode) << "\n";
            return false;
        }
    }
#else
    *portSai = ESAL_UNITTEST_MAGIC_NUM;
#endif

    // Get id from oid
    uint16_t _portId = (uint16_t)GET_OID_VAL(*portSai);


    // Store first in shadow area, and then bump count. 
    //
    portTable[portTableSize].portSai = *portSai;
    portTable[portTableSize].portId = _portId;
    portTableSize++;

    return true; 

}

bool esalAddAclToPort(sai_object_id_t portSai, sai_object_id_t aclSai, bool ingr){

    // Grab mutex.
    //
    std::unique_lock<std::mutex> lock(portTableMutex);

#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "API Query Fail in esalPortTableAddEntry\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return false; 
    }

    // Instantiate a new port sai. Mandatory attributes 
    // Adding port speed now as 1G default, but assuming it will change.
    //
    sai_attribute_t attr;
    attr.id = (ingr ? SAI_PORT_ATTR_INGRESS_ACL : SAI_PORT_ATTR_EGRESS_ACL);
    attr.value.oid = aclSai;

    // Create a port.
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "create_port Fail in esalPortTableAddEntry\n"));
        std::cout << "set port fail: " << esalSaiError(retcode) << "\n";
        return false; 
    }

#endif
    return true;
}


int VendorSetPortRate(
    uint16_t port, bool autoneg, vendor_speed_t speed, vendor_duplex_t duplex) {

    std::cout << __PRETTY_FUNCTION__ << " " << port << std::endl;
    int rc  = ESAL_RC_OK;
#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAutoNeg;
        val.SFPVal.AutoNeg = autoneg;
        values.push_back(val); 
        val.SFPAttr = SFPSpeed;
        val.SFPVal.LinkSpeed = speed;
        values.push_back(val); 
        val.SFPAttr = SFPDuplex;
        val.SFPVal.LinkDuplex = duplex;
        values.push_back(val); 
        if (!esalSFPSetPort) return ESAL_RC_FAIL;
        esalSFPSetPort(port, values.size(), values.data());
    }

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#endif
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in VendorSetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_PORT_ATTR_SPEED;

    switch (speed) {
        case VENDOR_SPEED_TEN:
            attr.value.u32 = 10;
            break;
        case VENDOR_SPEED_HUNDRED:
            attr.value.u32 = 100;
            break;
        case VENDOR_SPEED_GIGABIT:
            attr.value.u32 = 1000;
            break;
        case VENDOR_SPEED_TEN_GIGABIT:
            attr.value.u32 = 10000;
            break;
        default:
            attr.value.u32 = 1000;
            break; 
    }

    attributes.push_back(attr); 
#ifdef NOT_SUPPORTED_BY_SAI
    attr.id = SAI_PORT_ATTR_FULL_DUPLEX_MODE;
    attr.value.booldata = (duplex == VENDOR_DUPLEX_FULL) ? true : false; 
    attributes.push_back(attr); 

    attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
    attr.value.booldata = autoneg;
    attributes.push_back(attr);
#else // NOT_SUPPORTED_BY_SAI
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    int cpssDuplexMode;
    if (duplex == VENDOR_DUPLEX_HALF)
        cpssDuplexMode = CPSS_PORT_HALF_DUPLEX_E;
    else
        cpssDuplexMode = CPSS_PORT_FULL_DUPLEX_E;

    int cppsAutoneg;
    if (autoneg)
        cppsAutoneg = 1;
    else
        cppsAutoneg = 0;

    if (speed == VENDOR_SPEED_TEN || speed == VENDOR_SPEED_HUNDRED || speed == VENDOR_SPEED_GIGABIT) {
        if (cpssDxChPortDuplexModeSet(devNum, portNum, cpssDuplexMode) != 0) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "VendorSetPortRate fail in cpssDxChPortDuplexModeSet\n"));
            std::cout << "VendorSetPortRate fail, for port: " << port << "\n";
            return ESAL_RC_FAIL;
        }
        if (cpssDxChPortInbandAutoNegEnableSet(devNum, portNum, cppsAutoneg) != 0) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "VendorSetPortRate fail in cpssDxChPortInbandAutoNegEnableSet\n"));
            std::cout << "VendorSetPortRate fail, for port: " << port << "\n";
            return ESAL_RC_FAIL;
        }
    }
#endif
#endif
    // Set the port attributes
    //
    for (auto &curAttr : attributes) {
        retcode = saiPortApi->set_port_attribute(portSai, &curAttr);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "set_port_attribute Fail in VendorSetPortRate\n"));
            std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        }
    }


#endif

    return rc;
}

int VendorGetPortRate(uint16_t port, vendor_speed_t *speed) {

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#endif
    int rc  = ESAL_RC_OK;

#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPSpeed;
        val.SFPVal.LinkSpeed = *speed;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(port, values.size(), values.data());
        *speed = values[0].SFPVal.LinkSpeed;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai = 0;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetPortRate\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_SPEED;
    attributes.push_back(attr); 

    // Set the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortRate\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    switch (attributes[0].value.u32) {
        case 10:
            *speed = VENDOR_SPEED_TEN;
            break;
        case 100:
            *speed = VENDOR_SPEED_HUNDRED;
            break;
        case 1000:
            *speed = VENDOR_SPEED_GIGABIT;
            break;
        case 10000:
            *speed = VENDOR_SPEED_TEN_GIGABIT;
            break;
        default:
            std::cout << "Switch statement speed fail: " << attr.value.u32 << "\n"; 
    }
#endif

    return rc;
}

int VendorGetPortDuplex(uint16_t port, vendor_duplex_t *duplex) {

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#endif
    int rc  = ESAL_RC_OK;
#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPDuplex;
        val.SFPVal.LinkDuplex = *duplex;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(port, values.size(), values.data());
        *duplex = values[0].SFPVal.LinkDuplex;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortDuplex\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetPortDuplex\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }
#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_FULL_DUPLEX_MODE;
    attributes.push_back(attr); 

    // Set the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortDuplex\n"));
        std::cout << "get_port_attr fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    *duplex = (attributes[0].value.booldata) ? VENDOR_DUPLEX_FULL : VENDOR_DUPLEX_HALF;
#else // NOT_SUPPORTED_BY_SAI
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    int cpssDuplexMode;
    if (cpssDxChPortDuplexModeGet(devNum, portNum, &cpssDuplexMode) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortDuplex fail in cpssDxChPortDuplexModeGet\n"));
        std::cout << "VendorGetPortDuplex fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
    *duplex = (cpssDuplexMode == CPSS_PORT_HALF_DUPLEX_E) ? VENDOR_DUPLEX_HALF : VENDOR_DUPLEX_FULL;
#endif
#endif
#endif

    return rc;
}

int VendorGetPortAutoNeg(uint16_t port, bool *aneg) {

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#endif

    int rc  = ESAL_RC_OK;
#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAutoNeg;
        val.SFPVal.AutoNeg = *aneg;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL; 
        esalSFPGetPort(port, values.size(), values.data());
        *aneg = values[0].SFPVal.AutoNeg;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortAutoNeg\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetPortAutoNeg\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }
#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_AUTO_NEG_MODE;
    attributes.push_back(attr); 

    // Set the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortAutoNeg\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    *aneg = attributes[0].value.booldata;
#else
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    int cpssAutoneg;
    if (cpssDxChPortInbandAutoNegEnableGet(devNum, portNum, &cpssAutoneg) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAutoNeg fail in cpssDxChPortInbandAutoNegEnableGet\n"));
        std::cout << "VendorGetPortAutoNeg fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
    *aneg = (cpssAutoneg) ? true : false;
#endif
#endif


#endif

    return rc;
}

int VendorGetPortLinkState(uint16_t port, bool *ls) {

#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#endif

    int rc  = ESAL_RC_OK;
#ifndef LARCH_ENVIRON
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPLinkStatus;
        val.SFPVal.LinkStatus = *ls;
        values.push_back(val); 
        if (!esalSFPGetPort) return ESAL_RC_FAIL; 
        esalSFPGetPort(port, values.size(), values.data());
        *ls = values[0].SFPVal.AutoNeg;
        return rc; 
    }
#endif    
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortLinkState\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetPortLinkState\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_OPER_STATUS;
    attributes.push_back(attr); 

    // Set the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortLinkState\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    *ls = (attributes[0].value.u32 == SAI_PORT_OPER_STATUS_UP) ? true : false;
#endif

    return rc;
}


int VendorEnablePort(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorEnablePort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorEnablePort\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    attr.value.booldata = true; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorEnablePort\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
#endif

    return rc;
}

int VendorDisablePort(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    int rc  = ESAL_RC_OK;

#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorDisablePort\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalPortTableFindSai fail in VendorDisablePort\n"));
        std::cout << "VendorAddMemberPort failport:" << port << "\n";
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADMIN_STATE;
    attr.value.booldata = false; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorDisablePort\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
#endif

    return rc;
}

int VendorSetFrameMax(uint16_t port, uint16_t size) {
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorSetFrameMax\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port. Create it if did not exist. 
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "esalPortTableAddEntry fail in VendorSetFrameMax\n"));
        std::cout << "VendorAddMemberPort failport:" << port << "\n";
        return ESAL_RC_FAIL;
    }

    // Add attributes. 
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_MTU;
    attr.value.u32 = size; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorSetFrameMax\n"));
        std::cout << "sai_port_attribute fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

#endif

    return ESAL_RC_OK;
}

int VendorGetFrameMax(uint16_t port, uint16_t *size) {
    
    int rc  = ESAL_RC_OK;
#ifdef DEBUG
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#endif

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS

    // Get api interface. 
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetFrameMax\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetFrameMax\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_MTU;
    attributes.push_back(attr); 

    // Set the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, attributes.size(), attributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "get_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    *size = attributes[0].value.u32;

#endif
    return rc;
}

int VendorSetPortAdvertAbility(uint16_t port, uint16_t cap) {
    
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
#ifndef LARCH_ENVIRON
    // Set Port Advertising Capability
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAdvertise;
        val.SFPVal.AdvertAbility = (vendor_port_advert_ability) cap;
        values.push_back(val); 
        if (!esalSFPSetPort) return ESAL_RC_FAIL; 
        esalSFPSetPort(port, values.size(), values.data());
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
#ifdef NOT_SUPPORTED_BY_SAI
    // FIXME:  The following attributes are not yet supported. 
    // Documented here ... http://rtx-swtl-jira.fnc.net.local/browse/LARCH-3
    //
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetFrameMax\n"));
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    int32_t speedarr[3];
    sai_s32_list_t speedlst;
    speedlst.list = speedarr;
    speedlst.count = 0;

    if (cap & VENDOR_PORT_ABIL_1000MB_FD) { 
        speedlst.list[speedlst.count++] = 1000;
    }
    
    if (cap & VENDOR_PORT_ABIL_100MB_FD) { 
        speedlst.list[speedlst.count++] = 100; 
    }
    
    if (cap & VENDOR_PORT_ABIL_10MB_FD) { 
        speedlst.list[speedlst.count++] = 10;
    }

    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADVERTISED_SPEED;
    attr.value.s32list = speedlst; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
    
    int32_t duplexarr[3];
    sai_s32_list_t duplexlst;
    duplexlst.list = duplexarr;
    duplexlst.count = 0;
    
    if (cap & VENDOR_PORT_ABIL_1000MB_HD) { 
        duplexlst.list[duplexlst.count++] = 1000;
    }
    
    if (cap & VENDOR_PORT_ABIL_100MB_HD) { 
        duplexlst.list[duplexlst.count++] = 100;
    }
    
    if (cap & VENDOR_PORT_ABIL_10MB_HD) { 
        duplexlst.list[duplexlst.count++] = 10;
    }

    attr.id = SAI_PORT_ATTR_ADVERTISED_HALF_DUPLEX_SPEED;
    attr.value.s32list = duplexlst; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorGetFrameMax\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
#else // NOT_SUPPORTED_BY_SAI
#if 0 //TODO
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    port_auto_neg_advertisment_t portAnAdvertismentPtr;
    if (cpssDxChPortAutoNegAdvertismentConfigGet(devNum, portNum, &portAnAdvertismentPtr) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAdvertAbility fail in cpssDxChPortAutoNegAdvertismentConfigGet\n"));
        std::cout << "VendorGetPortAdvertAbility fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
    *advert = 0;
    if (portAnAdvertismentPtr.duplex == CPSS_PORT_FULL_DUPLEX_E) {
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_FD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_FD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_FD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
    else { // CPSS_PORT_HALF_DUPLEX_E
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_HD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_HD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_HD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
#endif
#endif
#endif
    
    return ESAL_RC_OK;
}

int VendorGetPortAdvertAbility(uint16_t port, uint16_t *advert) {
    
    std::cout << __PRETTY_FUNCTION__ << " " << port  << std::endl;
    
#ifndef LARCH_ENVIRON
    int rc  = ESAL_RC_OK;
    // First check to see if supported by SFP library.
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(port)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPAdvertise;
        val.SFPVal.AdvertAbility = *advert;
        values.push_back(val); 
        if (esalSFPGetPort) return ESAL_RC_FAIL;
        esalSFPGetPort(port, values.size(), values.data());
        *advert = values[0].SFPVal.AdvertAbility;
        return rc; 
    }
#endif
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorGetPortAdvertAbility\n"));
        std::cout << "sai_api_query fail: " <<  esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorGetPortAdvertAbility\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

#ifdef NOT_SUPPORTED_BY_SAI
    // Add attributes. 
    //
    std::vector<sai_attribute_t> spdattributes;
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_ADVERTISED_SPEED;
    spdattributes.push_back(attr); 

    // Get the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, spdattributes.size(), spdattributes.data());
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortAdvertAbility\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    *advert = 0; 
    auto spdlst = spdattributes[0].value.s32list; 
    for(uint32_t spdi = 0; spdi < spdlst.count; spdi++) {
        switch (spdlst.list[spdi]) {
            case 10:
                *advert |= VENDOR_PORT_ABIL_10MB_FD;
                break;
            case 100:
                *advert |= VENDOR_PORT_ABIL_100MB_FD;
                break;
            case 1000:
                *advert |= VENDOR_PORT_ABIL_1000MB_FD;
                break;
            default: 
                std::cout << "Unknown full duplex speed: " << spdlst.list[spdi] << "\n";
        }
    }

    // Add attributes. 
    //
    std::vector<sai_attribute_t> dupattributes;
    attr.id = SAI_PORT_ATTR_ADVERTISED_HALF_DUPLEX_SPEED;
    dupattributes.push_back(attr); 

    // Get the port attributes
    //
    retcode = saiPortApi->get_port_attribute(portSai, dupattributes.size(), dupattributes.data());

    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "get_port_attribute fail in VendorGetPortAdvertAbility\n"));
        std::cout << "set_port fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
    
    auto duplst = spdattributes[0].value.s32list; 
    for(uint32_t dupi = 0; dupi < duplst.count; dupi++) {
        switch (duplst.list[dupi]) {
            case 10:
                *advert |= VENDOR_PORT_ABIL_10MB_HD;
                break;
            case 100:
                *advert |= VENDOR_PORT_ABIL_100MB_HD;
                break;
            case 1000:
                *advert |= VENDOR_PORT_ABIL_1000MB_HD;
                break;
            default: 
                std::cout << "Unknown half speed: " << duplst.list[dupi] << "\n";
        }
    }
#else // NOT_SUPPORTED_BY_SAI
#ifdef HAVE_MRVL
    // XXX Direct cpss calls in Legacy mode, PortManager does not aware of this.
    // Be carefull
    uint32_t devNum = 0;
    // Get portNum from oid
    uint16_t portNum = (uint16_t)GET_OID_VAL(portSai);
    CPSS_DXCH_PORT_AUTONEG_ADVERTISMENT_STC portAnAdvertismentPtr;
    if (cpssDxChPortAutoNegAdvertismentConfigGet(devNum, portNum, &portAnAdvertismentPtr) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorGetPortAdvertAbility fail in cpssDxChPortAutoNegAdvertismentConfigGet\n"));
        std::cout << "VendorGetPortAdvertAbility fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
    *advert = 0;
    if (portAnAdvertismentPtr.duplex == CPSS_PORT_FULL_DUPLEX_E) {
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_FD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_FD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_FD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
    else { // CPSS_PORT_HALF_DUPLEX_E
        switch (portAnAdvertismentPtr.speed) {
        case CPSS_PORT_SPEED_10_E:
            *advert |= VENDOR_PORT_ABIL_10MB_HD;
            break;
        case CPSS_PORT_SPEED_100_E:
            *advert |= VENDOR_PORT_ABIL_100MB_HD;
            break;
        case CPSS_PORT_SPEED_1000_E:
            *advert |= VENDOR_PORT_ABIL_1000MB_HD;
            break;
        default:
            std::cout << "Unknown speed: \n";
            return ESAL_RC_FAIL;
        }
    }
#endif
#endif
#endif

    return ESAL_RC_OK;
}

VendorL2ParamChangeCb_fp_t portStateChangeCb = 0;
void *portStateCbData = 0; 

int VendorRegisterL2ParamChangeCb(VendorL2ParamChangeCb_fp_t cb, void *cbId) {
    std::cout << __PRETTY_FUNCTION__ << std::endl;

    // Register global callback used for autoneg and link status. 
    //
    std::unique_lock<std::mutex> lock(portTableMutex);
    portStateChangeCb = cb;
    portStateCbData = cbId;
#ifndef LARCH_ENVIRON
    if (!esalSFPRegisterL2ParamChangeCb || 
         esalSFPRegisterL2ParamChangeCb(cb, cbId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "SFPRegisterL2ParamChangeCb fail in VendorRegisterL2ParamChangeCb\n"));
        std::cout << "VendorRegisterL2ParamChangeCb fail\n";
        return ESAL_RC_FAIL;
    }
#endif
    return ESAL_RC_OK;
}

void esalPortTableState(sai_object_id_t portSai, bool portState){


    // Verify if port exists within our provisioning. 
    //
    uint16_t portId;
    if (!esalPortTableFindId(portSai, &portId)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindId fail in esalPortTableState\n"));
        std::cout << "esalPortTableFindSai fail: " << portId << "\n";
        return; 
    }

#ifndef LARCH_ENVIRON
    // Check to see if the SFP supported by SFP Library. If so, just call
    // set link state, and let SFP library handle callback. 
    //
    if (esalSFPLibrarySupport && esalSFPLibrarySupport(portId)) {
        std::vector<SFPAttribute> values;
        SFPAttribute val;
        val.SFPAttr = SFPLinkStatus;
        val.SFPVal.LinkStatus = portState;
        values.push_back(val); 
        if (!esalSFPSetPort) return; 
        esalSFPSetPort(portId, values.size(), values.data());
        return; 
    }
#endif
    if (!portStateChangeCb) return;

    vendor_speed_t speed;
    bool autoneg;
    vendor_duplex_t duplex;

    if (VendorGetPortRate(portId, &speed) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortRate fail in esalPortTableState\n"));
        std::cout << "VendorGetPortRate fail: " << portId << "\n";
    }

    if (VendorGetPortAutoNeg(portId, &autoneg) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortAutoNeg fail in esalPortTableState\n"));
        std::cout << "VendorGetPortAutoNeg fail: " << portId << "\n";
    }

    if (VendorGetPortDuplex(portId, &duplex) != ESAL_RC_OK) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorGetPortDuplex fail in esalPortTableState\n"));
        std::cout << "VendorGetPortDuplex fail: " << portId << "\n";
    }


    portStateChangeCb(portStateCbData, portId, portState, autoneg, speed, duplex);

}


int VendorResetPort(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << port << " is NYI" << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
    VendorDisablePort(port);
    VendorEnablePort(port);
    return ESAL_RC_OK;
}

int VendorReadReg(uint16_t port, uint16_t reg, uint16_t *val) {
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifdef HAVE_MRVL
    uint32_t devNum = 0;
    if (cpssDxChPhyPortSmiRegisterRead(devNum, port, reg, val) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorReadReg fail in cpssDxChPhyPortSmiRegisterRead\n"));
        std::cout << "VendorReadReg fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
#endif
    return ESAL_RC_OK;
}

int VendorWriteReg(uint16_t port, uint16_t reg, uint16_t val) {
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }
#ifdef HAVE_MRVL
    uint32_t devNum = 0;
    if (cpssDxChPhyPortSmiRegisterWrite(devNum, port, reg, val) != 0) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "VendorWriteReg fail in cpssDxChPhyPortSmiRegisterWrite\n"));
        std::cout << "VendorWriteReg fail, for port: " << port << "\n";
        return ESAL_RC_FAIL;
    }
#endif
    return ESAL_RC_OK;
}

int VendorDropTaggedPacketsOnIngress(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << " " << port << " " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

#ifndef UTS
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_DROP_TAGGED;
    attr.value.booldata = true; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorDropTaggedPacketsOnIngress\n"));
        std::cout << "set_port fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
#endif

    return ESAL_RC_OK;
}

int VendorDropUntaggedPacketsOnIngress(uint16_t port) {
    std::cout << __PRETTY_FUNCTION__ << " " << port << " " << std::endl;
    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

#ifndef UTS
    
    // Get port table api
    //
    sai_status_t retcode;
    sai_port_api_t *saiPortApi;
    retcode =  sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "sai_api_query fail in VendorDropUntaggedPacketsOnIngress\n"));
        std::cout << "sai_api_query fail:" << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }

    // Find the sai port.
    //
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(port, &portSai)) {
        std::cout << "esalPortTableFindSai fail: " << port << "\n";
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "esalPortTableFindSai fail in VendorDropUntaggedPacketsOnIngress\n"));
        return ESAL_RC_FAIL; 
    }

    // Add attributes. 
    //
    sai_attribute_t attr;
    attr.id = SAI_PORT_ATTR_DROP_UNTAGGED;
    attr.value.booldata = true; 
 
    // Set the port attributes
    //
    retcode = saiPortApi->set_port_attribute(portSai, &attr);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
            SWERR_FILELINE, "set_port_attribute fail in VendorDropUntaggedPacketsOnIngress\n"));
        std::cout << "set_port fail: " << esalSaiError(retcode) << "\n";
        return ESAL_RC_FAIL; 
    }
#endif


    return ESAL_RC_OK;
}


}

