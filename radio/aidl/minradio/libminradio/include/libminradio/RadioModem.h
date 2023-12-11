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
#pragma once

#include <libminradio/GuaranteedCallback.h>

#include <aidl/android/hardware/radio/modem/BnRadioModem.h>

namespace android::hardware::radio::minimal {

class RadioModem : public aidl::android::hardware::radio::modem::BnRadioModem {
  public:
    RadioModem();

  protected:
    ::ndk::ScopedAStatus enableModem(int32_t serial, bool on) override;
    ::ndk::ScopedAStatus getModemStackStatus(int32_t serial) override;
    ::ndk::ScopedAStatus nvResetConfig(
            int32_t serial, ::aidl::android::hardware::radio::modem::ResetNvType type) override;
    ::ndk::ScopedAStatus nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>& prl) override;
    ::ndk::ScopedAStatus requestShutdown(int32_t serial) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus sendDeviceState(
            int32_t serial, ::aidl::android::hardware::radio::modem::DeviceStateType stateType,
            bool state) override;
    ::ndk::ScopedAStatus setRadioCapability(
            int32_t s, const ::aidl::android::hardware::radio::modem::RadioCapability& rc) override;
    ::ndk::ScopedAStatus setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                       bool preferredForEmergencyCall) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemResponse>&
                    radioModemResponse,
            const std::shared_ptr<::aidl::android::hardware::radio::modem::IRadioModemIndication>&
                    radioModemIndication) override;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    ::ndk::ScopedAStatus getDeviceIdentity(int32_t serial) override;
    ::ndk::ScopedAStatus nvReadItem(
            int32_t serial, ::aidl::android::hardware::radio::modem::NvItem itemId) override;
    ::ndk::ScopedAStatus nvWriteItem(
            int32_t serial, const ::aidl::android::hardware::radio::modem::NvWriteItem& i) override;
#pragma clang diagnostic pop

    GuaranteedCallback<::aidl::android::hardware::radio::modem::IRadioModemIndication,
                       ::aidl::android::hardware::radio::modem::IRadioModemIndicationDefault, true>
            indicate;
    GuaranteedCallback<::aidl::android::hardware::radio::modem::IRadioModemResponse,
                       ::aidl::android::hardware::radio::modem::IRadioModemResponseDefault>
            respond;
};

}  // namespace android::hardware::radio::minimal
