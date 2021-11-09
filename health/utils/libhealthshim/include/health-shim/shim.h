/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <map>

#include <aidl/android/hardware/health/IHealth.h>
#include <android/hardware/health/2.0/IHealth.h>

namespace aidl::android::hardware::health {

class HealthShim : public IHealth {
    using HidlHealth = ::android::hardware::health::V2_0::IHealth;
    using HidlHealthInfoCallback = ::android::hardware::health::V2_0::IHealthInfoCallback;

  public:
    explicit HealthShim(const ::android::sp<HidlHealth>& service);

    ::ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHealthInfoCallback>& in_callback) override;
    ::ndk::ScopedAStatus unregisterCallback(
            const std::shared_ptr<IHealthInfoCallback>& in_callback) override;
    ::ndk::ScopedAStatus update() override;
    ::ndk::ScopedAStatus getChargeCounterUah(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getCurrentNowMicroamps(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getCurrentAverageMicroamps(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getCapacity(int32_t* _aidl_return) override;
    ::ndk::ScopedAStatus getEnergyCounterNwh(int64_t* _aidl_return) override;
    ::ndk::ScopedAStatus getChargeStatus(BatteryStatus* _aidl_return) override;
    ::ndk::ScopedAStatus getStorageInfo(std::vector<StorageInfo>* _aidl_return) override;
    ::ndk::ScopedAStatus getDiskStats(std::vector<DiskStats>* _aidl_return) override;
    ::ndk::ScopedAStatus getHealthInfo(HealthInfo* _aidl_return) override;

  private:
    sp<HidlHealth> service_;
    std::map<std::shared_ptr<IHealthInfoCallback>, sp<HidlHealthInfoCallback>> callback_map_;
};

}  // namespace aidl::android::hardware::health
