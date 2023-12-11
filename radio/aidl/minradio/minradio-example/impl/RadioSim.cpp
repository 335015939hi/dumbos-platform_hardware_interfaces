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

#include "RadioSim.h"

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "SimImpl"

namespace android::hardware::radio::service {

using minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::sim;
namespace aidlConfig = ::aidl::android::hardware::radio::config;
constexpr auto ok = &ScopedAStatus::ok;

RadioSim::RadioSim() {
    setIccid("98683081462002318379");
}

ScopedAStatus RadioSim::getIccCardStatus(int32_t serial) {
    LOG_CALL;

    // TODO: try saying it's absent?
    aidl::CardStatus cardStatus{
            .cardState = aidl::CardStatus::STATE_PRESENT,
            .universalPinState = aidl::PinState::DISABLED,
            .gsmUmtsSubscriptionAppIndex = 0,
            .cdmaSubscriptionAppIndex = -1,
            .imsSubscriptionAppIndex = -1,
            .applications =
                    {
                            aidl::AppStatus{
                                    .appType = aidl::AppStatus::APP_TYPE_USIM,
                                    .appState = aidl::AppStatus::APP_STATE_READY,
                                    .persoSubstate = aidl::PersoSubstate::READY,
                            },
                    },
            .atr = "",
            .iccid = mIccid.value_or(""),
            .eid = "eUICC-simslot1",  // TODO: same or different than getSimSlotsStatus
            .slotMap =
                    {
                            .physicalSlotId = 0,
                            .portId = 0,
                    },
            .supportedMepMode = aidlConfig::MultipleEnabledProfilesMode::NONE,
    };
    respond->getIccCardStatusResponse(noError(serial), cardStatus);
    return ok();
}

ScopedAStatus RadioSim::getImsiForApp(int32_t serial, const std::string& aid) {
    LOG_CALL << aid;
    // 6-digit IMSI prefix has to be a valid mccmnc
    respond->getImsiForAppResponse(noError(serial), "311740123456789");
    return ok();
}

}  // namespace android::hardware::radio::service
