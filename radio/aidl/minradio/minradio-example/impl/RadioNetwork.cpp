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

#include "RadioNetwork.h"

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "NetworkImpl"

namespace android::hardware::radio::service {

using minimal::noError;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::network;
namespace aidlRadio = ::aidl::android::hardware::radio;
constexpr auto ok = &ScopedAStatus::ok;

// TODO: move to some common library
constexpr int kRadioIntMax = 0x7FFFFFFF;

ScopedAStatus RadioNetwork::getDataRegistrationState(int32_t serial) {
    LOG_CALL;
    aidl::CellIdentityLte cellid{
            .mcc = "300",
            .mnc = "555",
            .ci = 12345,
            .pci = 102,
            .tac = kRadioIntMax,
            .earfcn = 103,
            .operatorNames =
                    {
                            .alphaLong = "Minradio",
                            .alphaShort = "MR",
                            .operatorNumeric = "300555",
                            .status = aidl::OperatorInfo::STATUS_CURRENT,
                    },
            .bandwidth = 104,
            .additionalPlmns = {},
            .csgInfo = std::nullopt,
            .bands =
                    {
                            aidl::EutranBands::BAND_1,
                            aidl::EutranBands::BAND_88,
                    },
    };
    aidl::RegStateResult res{
            .regState = aidl::RegState::REG_HOME,
            .rat = aidlRadio::RadioTechnology::LTE,
            .reasonForDenial = aidl::RegistrationFailCause::NONE,
            .cellIdentity = cellid,
            .registeredPlmn = "300555",
    };
    respond->getDataRegistrationStateResponse(noError(serial), res);
    return ok();
}

ScopedAStatus RadioNetwork::getOperator(int32_t serial) {
    LOG_CALL;
    respond->getOperatorResponse(noError(serial),
                                 /*longName*/ "Minradio",
                                 /*shortName*/ "MR",
                                 /*numeric*/ "300555");
    return ok();
}

ScopedAStatus RadioNetwork::getSignalStrength(int32_t serial) {
    LOG_CALL;

    aidl::GsmSignalStrength gsmSS{
            60,  // (0-61, 99)
            0,   // (0-7, 99)
            kRadioIntMax,
    };
    aidl::CdmaSignalStrength cdmaSS{
            75,
            125,
    };
    aidl::EvdoSignalStrength evdoSS{
            75,
            125,
            7,
    };
    aidl::LteSignalStrength lteSS{
            30,   // (0-31, 99)
            100,  // Range: 44 to 140 dBm
            10,   // Range: 20 to 3 dB
            100, 10, kRadioIntMax, kRadioIntMax,
    };
    aidl::TdscdmaSignalStrength tdscdmaSS{
            kRadioIntMax,
            kRadioIntMax,
            kRadioIntMax,
    };
    aidl::WcdmaSignalStrength wcdmaSS{
            kRadioIntMax,
            kRadioIntMax,
            kRadioIntMax,
            kRadioIntMax,
    };
    aidl::NrSignalStrength nrSS{
            kRadioIntMax, kRadioIntMax, kRadioIntMax, kRadioIntMax, kRadioIntMax,
            kRadioIntMax, kRadioIntMax, {},           kRadioIntMax,
    };

    aidl::SignalStrength ss{
            gsmSS, cdmaSS, evdoSS, lteSS, tdscdmaSS, wcdmaSS, nrSS,
    };

    respond->getSignalStrengthResponse(noError(serial), ss);
    return ok();
}

}  // namespace android::hardware::radio::service
