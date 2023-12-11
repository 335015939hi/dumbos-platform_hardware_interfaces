/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <libminradio/RadioConfig.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "Config"

namespace android::hardware::radio::minimal {

using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::config;
constexpr auto ok = &ScopedAStatus::ok;

RadioConfig::RadioConfig() {}

ScopedAStatus RadioConfig::getHalDeviceCapabilities(int32_t serial) {
    LOG_CALL;
    respond->getHalDeviceCapabilitiesResponse(noError(serial), /* modemReducedFeatureSet1 */ true);
    return ok();
}

ScopedAStatus RadioConfig::getNumOfLiveModems(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioConfig::getPhoneCapability(int32_t serial) {
    LOG_CALL;
    aidl::PhoneCapability cap{
            .maxActiveData = 1,
            .maxActiveInternetData = 1,
            .isInternetLingeringSupported = false,
            .logicalModemIds = {0},
    };
    respond->getPhoneCapabilityResponse(noError(serial), cap);
    return ok();
}

ScopedAStatus RadioConfig::setNumOfLiveModems(int32_t serial, int8_t numOfLiveModems) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioConfig::setPreferredDataModem(int32_t serial, int8_t modemId) {
    LOG_CALL << modemId;
    respond->setPreferredDataModemResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioConfig::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioConfigResponse>& response,
        const std::shared_ptr<aidl::IRadioConfigIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;
    return ok();
}

ScopedAStatus RadioConfig::setSimSlotsMapping(  //
        int32_t serial, const std::vector<aidl::SlotPortMapping>& slotMap) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

}  // namespace android::hardware::radio::minimal
