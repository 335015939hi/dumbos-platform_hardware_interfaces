/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <libminradio/RadioModem.h>

#include <libminradio/debug.h>
#include <libminradio/response.h>

#define RADIO_MODULE "Modem"

namespace android::hardware::radio::minimal {

using ::aidl::android::hardware::radio::RadioIndicationType;
using ::ndk::ScopedAStatus;
namespace aidl = ::aidl::android::hardware::radio::modem;
constexpr auto ok = &ScopedAStatus::ok;

RadioModem::RadioModem() {}

ScopedAStatus RadioModem::enableModem(int32_t serial, bool on) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::getModemStackStatus(int32_t serial) {
    LOG_CALL;
    respond->getModemStackStatusResponse(noError(serial), true);
    return ok();
}

ScopedAStatus RadioModem::nvResetConfig(int32_t serial, aidl::ResetNvType resetType) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>& prl) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::requestShutdown(int32_t serial) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::responseAcknowledgement() {
    LOG_CALL_NOSERIAL;
    return ok();
}

ScopedAStatus RadioModem::sendDeviceState(int32_t serial, aidl::DeviceStateType type, bool state) {
    LOG_CALL;
    respond->sendDeviceStateResponse(noError(serial));
    return ok();
}

ScopedAStatus RadioModem::setRadioCapability(int32_t serial, const aidl::RadioCapability& rc) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                        bool preferredForEmergencyCall) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::setResponseFunctions(
        const std::shared_ptr<aidl::IRadioModemResponse>& response,
        const std::shared_ptr<aidl::IRadioModemIndication>& indication) {
    LOG_CALL_NOSERIAL << response << ' ' << indication;
    CHECK(response);
    CHECK(indication);
    respond = response;
    indicate = indication;

    indicate->rilConnected(RadioIndicationType::UNSOLICITED);
    indicate->radioStateChanged(RadioIndicationType::UNSOLICITED, aidl::RadioState::ON);

    return ok();
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

// TODO: pull data from getImei? Or make framework not call this
ScopedAStatus RadioModem::getDeviceIdentity(int32_t serial) {
    LOG_CALL;
    respond->getDeviceIdentityResponse(noError(serial),
                                       /* imei */ "867400022047199",
                                       /* imeisv */ "01",
                                       /* esn */ "",
                                       /* meid */ "");
    return ok();
}

ScopedAStatus RadioModem::nvReadItem(int32_t serial, aidl::NvItem itemId) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

ScopedAStatus RadioModem::nvWriteItem(int32_t serial, const aidl::NvWriteItem& item) {
    LOG_NOT_IMPLEMENTED;
    return ok();
}

#pragma clang diagnostic pop

}  // namespace android::hardware::radio::minimal
