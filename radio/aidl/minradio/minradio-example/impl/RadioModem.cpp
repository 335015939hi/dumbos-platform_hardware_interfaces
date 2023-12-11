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

#include "RadioModem.h"

#include <aidl/android/hardware/radio/RadioAccessFamily.h>
#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "ModemImpl"

namespace android::hardware::radio::service {

using ::aidl::android::hardware::radio::AccessNetwork;
using ::aidl::android::hardware::radio::RadioTechnology;
using minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::modem;
namespace aidlRadio = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

ScopedAStatus RadioModem::getBasebandVersion(int32_t serial) {
    LOG_CALL;
    respond->getBasebandVersionResponse(noError(serial), "1.0.0.0");
    return ok();
}

ScopedAStatus RadioModem::getHardwareConfig(int32_t serial) {
    LOG_CALL;

    aidl::HardwareConfig modem1Config{
            .type = aidl::HardwareConfig::TYPE_MODEM,
            .uuid = "uuid-modem1",
            .state = aidl::HardwareConfig::STATE_ENABLED,
            .modem = {{
                    .rilModel = 0,
                    .rat = RadioTechnology::LTE,
                    .maxVoiceCalls = 0,
                    .maxDataCalls = 1,
                    .maxStandby = 1,
            }},
    };

    aidl::HardwareConfig sim1Config{
            .type = aidl::HardwareConfig::TYPE_SIM,  // <clang-format trigger>
            .uuid = "uuid-sim1",
            .state = aidl::HardwareConfig::STATE_ENABLED,
            .sim = {{
                    .modemUuid = "uuid-modem1",
            }},
    };

    respond->getHardwareConfigResponse(noError(serial), {modem1Config, sim1Config});
    return ok();
}

ScopedAStatus RadioModem::getModemActivityInfo(int32_t serial) {
    LOG_CALL;
    aidl::ActivityStatsTechSpecificInfo someInfo{
            .rat = AccessNetwork::EUTRAN,
            .frequencyRange = aidl::ActivityStatsTechSpecificInfo::FREQUENCY_RANGE_MID,
            .txmModetimeMs = {0, 0, 0, 1, 0},
            .rxModeTimeMs = 1,
    };
    aidl::ActivityStatsInfo info{
            .sleepModeTimeMs = 0,
            .idleModeTimeMs = 0,
            .techSpecificInfo = {someInfo},
    };
    respond->getModemActivityInfoResponse(noError(serial), info);
    return ok();
}

ScopedAStatus RadioModem::getRadioCapability(int32_t serial) {
    LOG_CALL;
    aidl::RadioCapability cap{
            .session = 0,
            .phase = 0,
            .raf = static_cast<int>(aidlRadio::RadioAccessFamily::LTE),  // bitmap
            .logicalModemUuid = "com.android.modem.minradio",
            .status = aidl::RadioCapability::STATUS_SUCCESS,
    };
    respond->getRadioCapabilityResponse(noError(serial), cap);
    return ok();
}

ScopedAStatus RadioModem::getImei(int32_t serial) {
    LOG_CALL;
    aidl::ImeiInfo info{
            .type = aidl::ImeiInfo::ImeiType::PRIMARY,
            .imei = "867400022047199",
            .svn = "01",
    };
    respond->getImeiResponse(noError(serial), info);
    return ok();
}

}  // namespace android::hardware::radio::service
