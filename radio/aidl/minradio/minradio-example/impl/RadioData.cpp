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

#include "RadioData.h"

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "DataImpl"

namespace android::hardware::radio::service {

using minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::data;
namespace aidlCommon = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioData::setupDataCall(int32_t serial, aidlCommon::AccessNetwork accessNetwork,
                                       const aidl::DataProfileInfo& dataProfileInfo,
                                       bool roamingAllowed, aidl::DataRequestReason reason,
                                       const std::vector<aidl::LinkAddress>& addresses,
                                       const std::vector<std::string>& dnses, int32_t pduSessId,
                                       const std::optional<aidl::SliceInfo>& sliceInfo,
                                       bool matchAllRuleAllowed) {
    LOG_CALL << static_cast<int>(accessNetwork)           //
             << " {" << dataProfileInfo.profileId << '}'  //
             << ' ' << roamingAllowed                     //
             << ' ' << static_cast<int>(reason)           //
             << ' ' << addresses.size()                   //
             << ' ' << dnses.size() << ' ' << pduSessId   //
             << ' ' << sliceInfo.has_value()              //
             << ' ' << matchAllRuleAllowed;

    // TODO: set IP address and up the interface
    auto res = system("ifconfig buried_eth0 192.168.97.2/30");
    LOG(INFO) << "system() result: " << res;

    aidl::SetupDataCallResult result{
            .cause = aidl::DataCallFailCause::NONE,
            .suggestedRetryTime = 0x7fffffffffffffff,
            .cid = 1234321,
            .active = aidl::SetupDataCallResult::DATA_CONNECTION_STATUS_ACTIVE,
            .type = aidl::PdpProtocolType::IP,
            .ifname = "buried_eth0",
            .addresses = {{
                    .address = "192.168.97.2/30",
                    .addressProperties = 0,
                    .deprecationTime = 0x7FFFFFFFFFFFFFFF,
                    .expirationTime = 0x7FFFFFFFFFFFFFFF,
            }},
            .dnses = {"8.8.8.8"},
            .gateways = {"192.168.97.1"},
            .pcscf = {},
            .mtuV4 = 0,
            .mtuV6 = 0,
            .defaultQos = 0,
            .qosSessions = {},
            .handoverFailureMode = aidl::SetupDataCallResult::HANDOVER_FAILURE_MODE_LEGACY,
            .pduSessionId = 0,
            .sliceInfo = std::nullopt,
            .trafficDescriptors = {},
    };

    respond->setupDataCallResponse(noError(serial), result);

    return ok();
}

}  // namespace android::hardware::radio::service
