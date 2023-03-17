/**
 * @file      esalSaiPolicer.cc
 * @copyright (C) 2022 Fujitsu Network Communications, Inc.
 * @brief     Support for esal-sai interface.
 *
 * @author    Spoorthi Manjunath
 * @date      1/2023
 *
 * Document Reference :
 */

#include "headers/esalSaiDef.h"
#include "headers/esalSaiUtils.h"

#include <iostream>

#include <string>
#include <cinttypes>
#include <map>
#include "esal_vendor_api/esal_vendor_api.h"

std::map<uint32_t, sai_object_id_t> bcPolicers;
std::map<uint32_t, sai_object_id_t> mcPolicers;

extern "C" {
#include "sai/saipolicer.h"

bool SetBroadcastRateLimiting(uint16_t pPort,
                     uint64_t bcastRateLimit, uint64_t bcastBurstLimit,
                     sai_object_id_t *bcSaiPolicer) {
    std::cout << "SetBroadcastRateLimiting: pPort=" << pPort << " bcastRateLimit="
              << bcastRateLimit << " bcastBurstLimit=" << bcastBurstLimit
              << " in kbps" << std::endl;

     // Get port table api
    sai_status_t retcode;
    sai_policer_api_t *saiPolicerApi;
    retcode =  sai_api_query(SAI_API_POLICER, (void**) &saiPolicerApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in SetBroadcastRateLimiting\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        std::cout << "SetBroadcastRateLimiting fail pPort: " << pPort << std::endl;
        return false; 
    }

    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_POLICER_ATTR_METER_TYPE;
    attr.value.s32 = SAI_METER_TYPE_BYTES;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_MODE;
    attr.value.s32 = SAI_POLICER_MODE_SR_TCM;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_COLOR_SOURCE;
    attr.value.s32 = SAI_POLICER_COLOR_SOURCE_AWARE;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_CIR;
    attr.value.u64 = bcastRateLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_PBS;
    attr.value.u64 = bcastBurstLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_CBS;
    attr.value.u64 = bcastBurstLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_GREEN_PACKET_ACTION;
    attr.value.s32 = SAI_PACKET_ACTION_FORWARD;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_RED_PACKET_ACTION;
    attr.value.s32 = SAI_PACKET_ACTION_DROP;
    attributes.push_back(attr);

/*    attr.id = SAI_POLICER_ATTR_ENABLE_COUNTER_PACKET_ACTION_LIST;
    attr.value.booldata = true;
    attributes.push_back(attr);
*/
    retcode = saiPolicerApi->create_policer(bcSaiPolicer,
                            esalSwitchId, attributes.size(), attributes.data());

    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in SetRateLimiting\n"));
        std::cout << "create_policer fail: retcode=" << retcode
            << " " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    return esalAddBroadcastPolicer(portSai, *bcSaiPolicer);
}

bool SetMulticastRateLimiting(uint16_t pPort,
                     uint64_t mcastRateLimit, uint64_t mcastBurstLimit,
                     sai_object_id_t *mcSaiPolicer) {
    std::cout << "SetMulticastRateLimiting: pPort=" << pPort
              << " mcastRateLimit=" << mcastRateLimit << " mcastBurstLimit="
              << mcastBurstLimit << " in kbps" << std::endl;

     // Get port table api
    sai_status_t retcode;
    sai_policer_api_t *saiPolicerApi;
    retcode =  sai_api_query(SAI_API_POLICER, (void**) &saiPolicerApi);
    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in SetMulticastRateLimiting\n"));
        std::cout << "sai_api_query fail: " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    // Find the sai port.
    sai_object_id_t portSai;
    if (!esalPortTableFindSai(pPort, &portSai)) {
        std::cout << "SetMulticastRateLimiting fail pPort: " << pPort << std::endl;
        return false; 
    }

    // Add attributes. 
    std::vector<sai_attribute_t> attributes;
    sai_attribute_t attr;

    attr.id = SAI_POLICER_ATTR_METER_TYPE;
    attr.value.s32 = SAI_METER_TYPE_BYTES;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_MODE;
    attr.value.s32 = SAI_POLICER_MODE_SR_TCM;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_COLOR_SOURCE;
    attr.value.s32 = SAI_POLICER_COLOR_SOURCE_AWARE;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_CIR;
    attr.value.u64 = mcastRateLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_PBS;
    attr.value.u64 = mcastBurstLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_CBS;
    attr.value.u64 = mcastBurstLimit * 125;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_GREEN_PACKET_ACTION;
    attr.value.s32 = SAI_PACKET_ACTION_FORWARD;
    attributes.push_back(attr);

    attr.id = SAI_POLICER_ATTR_RED_PACKET_ACTION;
    attr.value.s32 = SAI_PACKET_ACTION_DROP;
    attributes.push_back(attr);

/*    attr.id = SAI_POLICER_ATTR_ENABLE_COUNTER_PACKET_ACTION_LIST;
    attr.value.booldata = true;
    attributes.push_back(attr);
*/
    retcode = saiPolicerApi->create_policer(mcSaiPolicer,
                            esalSwitchId, attributes.size(), attributes.data());

    if (retcode) {
        SWERR(Swerr(Swerr::SwerrLevel::KS_SWERR_ONLY,
              SWERR_FILELINE, "sai_api_query Fail in SetMulticastRateLimiting\n"));
        std::cout << "create_policer fail: retcode=" << retcode
            << " " << esalSaiError(retcode)
                  << std::endl;
        return false; 
    }

    return esalAddMulticastPolicer(portSai, *mcSaiPolicer);
}

void processRateLimitsInit(uint32_t lPort) {
    EsalSaiUtils::rateLimit_t rLimits;
    uint32_t pPort;
    uint32_t dev;

    std::cout << "processRateLimitsInit called for lPort=" << lPort << std::endl;
    if (saiUtils.GetRateLimitInfo(lPort, dev, pPort, rLimits)) {
        if (rLimits.has_vals) {
            if (bcPolicers.find(lPort) == bcPolicers.end()) {
                sai_object_id_t bcSaiPolicer;
                if (SetBroadcastRateLimiting(pPort,
                            rLimits.bcastRateLimit, rLimits.bcastBurstLimit,
                            &bcSaiPolicer)) {
                    bcPolicers[lPort] = bcSaiPolicer;
                }
            }

            if (mcPolicers.find(lPort) == mcPolicers.end()) {
                sai_object_id_t mcSaiPolicer;
                if (SetMulticastRateLimiting(pPort,
                            rLimits.mcastRateLimit, rLimits.mcastBurstLimit,
                            &mcSaiPolicer)) {
                    mcPolicers[lPort] = mcSaiPolicer;
                }
            }
        }
    }
}
};
