/**
 * @file      esalSaiMc.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface. Multicast issues.
 *
 * @author    Steven Simmons
 * @date      3/2022
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <inttypes.h>

#include "sai/sai.h"
#include "sai/saistp.h"
#include "esal_vendor_api/esal_vendor_api.h"

// #define MC_DEBUG

struct mc_info {
    sai_object_id_t ms_oid;
    std::set<uint16_t> ports_in;
};
static std::map<uint16_t, mc_info> mcReplicatorTab;

#ifdef MC_DEBUG
static void printMcReplicatorTab () {
    for (auto &mcRow : mcReplicatorTab) {
        std::cout << "dst port: " << mcRow.first << " oid: "
                  << mcRow.second.ms_oid << " src ports: "<< std::endl;
        for (auto &port_in : mcRow.second.ports_in) {
            std::cout << port_in << " ";
        }
        std::cout << std::endl;
    }
}
#endif

#ifndef UTS
// Returns mirror session oids that's associated with the port
//
static std::set<sai_object_id_t> getPortMirrorSessionsList(uint16_t port) {
    std::set<sai_object_id_t> ms_list;
    for (auto &mcRow : mcReplicatorTab ) {
        if (mcRow.second.ports_in.count(port)) {
            ms_list.insert(mcRow.second.ms_oid);
        }
    }
    return ms_list;
}
#endif

extern "C" {
int VendorSetPortEgress(uint16_t lPort, uint16_t numPorts,
                        const uint16_t ports[]) {
    std::cout << __PRETTY_FUNCTION__ << " lPort=" << lPort  << std::endl;
    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }

    uint32_t dev;
    uint32_t pPort;
    if (!saiUtils.GetPhysicalPortInfo(lPort, &dev, &pPort)) {
        std::cout << "VendorSetPortEgress failed to get pPort, "
                  << "lPort= " << lPort << std::endl;
        return ESAL_RC_FAIL;
    }

    int rc  = ESAL_RC_OK;

    sai_status_t retcode;

    for (size_t i = 0; i < numPorts; i++) {
        if (mcReplicatorTab.count(ports[i]) == 0 ||
            mcReplicatorTab[ports[i]].ports_in.count(pPort) == 0) {
            retcode = VendorMirrorPort(pPort, ports[i]);
            if (retcode) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "VendorMirrorPort Fail" \
                                    "in VendorSetPortEgress\n"));
                std::cout << "VendorMirrorPort fail: " << esalSaiError(retcode)
                          << std::endl;
                return ESAL_RC_FAIL;
            }
        }
    }

    return rc;
}

int VendorMirrorPort(uint16_t srclPort, uint16_t dstlPort) {
#ifndef UTS
    std::cout << __PRETTY_FUNCTION__ << " srclPort:" << srclPort << " "
              << " dstlPort"  << dstlPort << std::endl;

    if (!useSaiFlag){
        return ESAL_RC_OK;
    }

    uint32_t srcDev;
    uint32_t srcpPort;
    if (!saiUtils.GetPhysicalPortInfo(srclPort, &srcDev, &srcpPort)) {
        std::cout << "VendorMirrorPort failed to get pPort, "
                  << "srclPort= " << srclPort << std::endl;
        return ESAL_RC_FAIL;
    }

    uint32_t dstDev;
    uint32_t dstpPort;
    if (!saiUtils.GetPhysicalPortInfo(dstlPort, &dstDev, &dstpPort)) {
        std::cout << "VendorMirrorPort failed to get pPort, "
                  << "dstlPort= " << dstlPort << std::endl;
        return ESAL_RC_FAIL;
    }
    int rc  = ESAL_RC_OK;

    sai_status_t retcode;
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    if (mcReplicatorTab.count(dstpPort) == 0) {

        sai_object_id_t mirror_session_oid_out;

        // Get mirror api
        //
        sai_mirror_api_t *saiMirrorApi;
        retcode = sai_api_query(SAI_API_MIRROR, (void**) &saiMirrorApi);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "sai_api_query Fail in VendorMirrorPort\n"));
            std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                      << std::endl;
            return ESAL_RC_FAIL;
        }

        // Create a dstPort for mirroring
        //
        attr.id = SAI_MIRROR_SESSION_ATTR_MONITOR_PORTLIST_VALID;
        attr.value.booldata = false;
        attributes.push_back(attr);

        attr.id = SAI_MIRROR_SESSION_ATTR_TYPE;
        attr.value.s32 = 0; // SPAN
        attributes.push_back(attr);

        attr.id = SAI_MIRROR_SESSION_ATTR_MONITOR_PORT;
        sai_object_id_t mirror_port_oid;
        if (!esalPortTableFindSai(dstpPort, &mirror_port_oid)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableFindSai Fail " \
                                "in VendorMirrorPort\n"));
            std::cout << "Failed to find oid for dstpPort: " << dstpPort
                      << std::endl;
            return ESAL_RC_FAIL;
        }
        attr.value.oid = mirror_port_oid;
        attributes.push_back(attr);

        retcode = saiMirrorApi->create_mirror_session(
            &mirror_session_oid_out, esalSwitchId, attributes.size(),attributes.data());
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "create_mirror_session Fail " \
                                        "in VendorMirrorPort\n"));
            std::cout << "create_mirror_session fail: " << esalSaiError(retcode)
                      << std::endl;
            return false;
        }

        mcReplicatorTab[dstpPort].ms_oid = mirror_session_oid_out;

    }

    if (mcReplicatorTab[dstpPort].ports_in.count(srcpPort) == 0) {

        // Get port table api
        //
        sai_port_api_t *saiPortApi;
        retcode = sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "sai_api_query Fail in VendorMirrorPort\n"));
            std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                      << std::endl;
            return ESAL_RC_FAIL;
        }

        // Set src port for mirroring
        //
        auto associated_ms = getPortMirrorSessionsList(srcpPort);
        std::vector<sai_object_id_t> dst_ms_list(
                        associated_ms.begin(), associated_ms.end());
        dst_ms_list.push_back(mcReplicatorTab[dstpPort].ms_oid);

        attr.id = SAI_PORT_ATTR_INGRESS_MIRROR_SESSION;
        attr.value.objlist.list = dst_ms_list.data();
        attr.value.objlist.count = (uint32_t)dst_ms_list.size();

        sai_object_id_t port_oid_in;
        if (!esalPortTableFindSai(srcpPort, &port_oid_in)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableFindSai Fail " \
                                "in VendorMirrorPort\n"));
            std::cout << "Failed to find oid for srcpPort: " << srcpPort
                      << std::endl;
            return ESAL_RC_FAIL;
        }

        retcode = saiPortApi->set_port_attribute(port_oid_in, &attr);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "set_port_attribute Fail " \
                                        "in VendorMirrorPort\n"));
            std::cout << "set_port_attribute fail: " << esalSaiError(retcode) << "\n";
            return ESAL_RC_FAIL;
        }

        mcReplicatorTab[dstpPort].ports_in.insert(srcpPort);

    }

    return rc;
#else
    return 0;
#endif
}

int VendorRemoveMirrorPort(uint16_t srclPort, uint16_t dstlPort) {
#ifndef UTS
    std::cout << __PRETTY_FUNCTION__ << " srclPort=" << srclPort << " "
              << " dstlPort=" << dstlPort << " is NYI" << std::endl;

    if (!useSaiFlag) {
        return ESAL_RC_OK;
    }

    uint32_t srcDev;
    uint32_t srcpPort;
    if (!saiUtils.GetPhysicalPortInfo(srclPort, &srcDev, &srcpPort)) {
        std::cout << "VendorRemoveMirrorPort failed to get pPort, "
                  << "srclPort= " << srclPort << std::endl;
        return ESAL_RC_FAIL;
    }

    uint32_t dstDev;
    uint32_t dstpPort;
    if (!saiUtils.GetPhysicalPortInfo(dstlPort, &dstDev, &dstpPort)) {
        std::cout << "VendorRemoveMirrorPort failed to get dstpPort, "
                  << "dstlPort= " << dstlPort << std::endl;
        return ESAL_RC_FAIL;
    }

    int rc  = ESAL_RC_OK;

#ifdef MC_DEBUG
    std::cout << "Mc before remove:" << std::endl;
    printMcReplicatorTab();
#endif

    sai_status_t retcode;

    if (mcReplicatorTab.count(dstpPort) == 1 &&
        mcReplicatorTab[dstpPort].ports_in.count(srcpPort) == 1) {

        sai_attribute_t attr;

        // Get port table api
        //
        sai_port_api_t *saiPortApi;
        retcode = sai_api_query(SAI_API_PORT, (void**) &saiPortApi);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "sai_api_query Fail " \
                                "in VendorRemoveMirrorPort\n"));
            std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                      << std::endl;
            return ESAL_RC_FAIL;
        }

        // Disable mirroring for srcPort
        //
        auto associated_ms = getPortMirrorSessionsList(srcpPort);
        associated_ms.erase(mcReplicatorTab[dstpPort].ms_oid);

        std::vector<sai_object_id_t> dst_ms_list(
                            associated_ms.begin(), associated_ms.end());
        
        attr.id = SAI_PORT_ATTR_INGRESS_MIRROR_SESSION;
        attr.value.objlist.list = dst_ms_list.data();
        attr.value.objlist.count = dst_ms_list.size();

        sai_object_id_t port_oid_in;
        if (!esalPortTableFindSai(srcpPort, &port_oid_in)) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                SWERR_FILELINE, "esalPortTableFindSai Fail " \
                                "in VendorRemoveMirrorPort\n"));
            std::cout << "Failed to find oid for port: " << srcpPort<< "\n";
            return ESAL_RC_FAIL;
        }

        retcode = saiPortApi->set_port_attribute(port_oid_in, &attr);
        if (retcode) {
            SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                        SWERR_FILELINE, "set_port_attribute Fail " \
                                        "in VendorRemoveMirrorPort\n"));
            std::cout << "set_port_attribute fail: " << esalSaiError(retcode) << "\n";
            return ESAL_RC_FAIL;
        }

        mcReplicatorTab[dstpPort].ports_in.erase(srcpPort);

        // Remove a mirror session if it's not used more
        //
        if (mcReplicatorTab[dstpPort].ports_in.size() == 0) {

            // Get mirror api
            //
            sai_mirror_api_t *saiMirrorApi;
            retcode = sai_api_query(SAI_API_MIRROR, (void**) &saiMirrorApi);
            if (retcode) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                    SWERR_FILELINE, "sai_api_query Fail " \
                                    "in VendorRemoveMirrorPort\n"));
                std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                          << "\n";
                return ESAL_RC_FAIL;
            }

            retcode = saiMirrorApi->remove_mirror_session(
                                    mcReplicatorTab[dstpPort].ms_oid);
            if (retcode) {
                SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
                            SWERR_FILELINE, "remove_mirror_session Fail " \
                                            "in VendorRemoveMirrorPort\n"));
                std::cout << "remove_mirror_session fail: "
                          << esalSaiError(retcode) << "\n";
                return ESAL_RC_FAIL;
            }

            mcReplicatorTab.erase(dstpPort);

        }

#ifdef MC_DEBUG
    std::cout << "Mc after remove:" << std::endl;
    printMcReplicatorTab();
#endif

    } else {
        std::cout << "Nothing to do! dstPort or srcPort not set or "
                  << "connected to each other for mirroring!" << std::endl;
        return ESAL_RC_FAIL;
    }

    return rc;
#else
    return 0;
#endif
}

}
