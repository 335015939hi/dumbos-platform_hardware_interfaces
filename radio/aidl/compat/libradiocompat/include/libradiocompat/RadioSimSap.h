/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "GuaranteedCallback.h"
#include "RadioSimSapCallback.h"

#include "/usr/local/google/home/poahlo/aosp-master-with-phones/out/soong/.intermediates/hardware/interfaces/radio/aidl/android.hardware.radio.sim-V2-ndk-source/gen/include/aidl/android/hardware/radio/sim/BnSap.h"
// #include <aidl/android/hardware/radio/sim/BnSap.h>
#include <android/hardware/radio/1.0/ISap.h>
#include <android/hardware/radio/1.0/ISapCallback.h>

namespace android::hardware::radio::compat {

/**
 * HAL translator from HIDL ISap to AIDL ISap
 *
 * This class wraps existing HIDL implementation (either a binder stub or real
 * class implementing the HAL) and implements AIDL HAL. It's up to the caller to
 * fetch source implementation and publish resulting HAL instance.
 */
class RadioSimSap : public aidl::android::hardware::radio::sim::BnSap {
    const sp<radio::V1_0::ISap> mSap;

    const sp<RadioSimSapCallback> mSapCallback;

    ::ndk::ScopedAStatus apduReq(int32_t token, SapApduType type,
                                 const std::vector<uint8_t>& command) override;
    ::ndk::ScopedAStatus connectReq(int32_t token, int32_t maxMsgSize) override;
    ::ndk::ScopedAStatus disconnectReq(int32_t token) override;
    ::ndk::ScopedAStatus powerReq(int32_t token, bool state) override;
    ::ndk::ScopedAStatus resetSimReq(int32_t token) override;
    ::ndk::ScopedAStatus setCallback(
            const std::shared_ptr<::aidl::android::hardware::radio::sim::ISapCallback>& sapCallback)
            override;
    ::ndk::ScopedAStatus setTransferProtocolReq(int32_t token,
                                                SapTransferProtocol transferProtocol) override;
    ::ndk::ScopedAStatus transferAtrReq(int32_t token) override;
    ::ndk::ScopedAStatus transferCardReaderStatusReq(int32_t token) override;

  public:
    /**
     * Constructs AIDL ISap instance wrapping existing HIDL ISap instance.
     *
     * \param hidlHal existing HIDL ISap HAL instance
     */
    RadioSimSap(sp<V1_0::ISap> hidlHal);
};

}  // namespace android::hardware::radio::compat
