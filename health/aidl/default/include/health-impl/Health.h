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

#include <memory>

#include <aidl/android/hardware/health/BnHealth.h>
#include <aidl/android/hardware/health/IHealthInfoCallback.h>
#include <healthd/BatteryMonitor.h>
#include <healthd/healthd.h>

namespace aidl {
namespace android {
namespace hardware {
namespace health {

// AIDL version of android::hardware::health::V2_1::implementation::Health
class Health : public BnHealth {
  public:
    Health(std::unique_ptr<healthd_config>&& config);

    ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override;
    ndk::ScopedAStatus unregisterCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override;
    ndk::ScopedAStatus update() override;
    ndk::ScopedAStatus getCapacity(int32_t* out) override;
    ndk::ScopedAStatus getChargeCounter(int32_t* out) override;
    ndk::ScopedAStatus getChargeStatus(BatteryStatus* out) override;
    ndk::ScopedAStatus getCurrentAverage(int32_t* out) override;
    ndk::ScopedAStatus getCurrentNow(int32_t* out) override;
    ndk::ScopedAStatus getDiskStats(std::vector<DiskStats>* out) override;
    ndk::ScopedAStatus getEnergyCounter(int64_t* out) override;
    ndk::ScopedAStatus getHealthConfig(HealthConfig* out) override;
    ndk::ScopedAStatus getHealthInfo(HealthInfo* out) override;
    ndk::ScopedAStatus getStorageInfo(std::vector<StorageInfo>* out) override;
    ndk::ScopedAStatus shouldKeepScreenOn(bool* out) override;

    binder_status_t dump(int fd, const char** args, uint32_t num_args) override;

  protected:
    // A subclass can override this to modify any health info object before
    // returning to clients. This is similar to healthd_board_battery_update().
    // By default, it does nothing.
    virtual void UpdateHealthInfo(HealthInfo* health_info);

  private:
    bool unregisterCallbackInternal(std::shared_ptr<IHealthInfoCallback> callback);

    ::android::BatteryMonitor battery_monitor_;
    std::unique_ptr<healthd_config> healthd_config_;

    std::mutex callbacks_lock_;
    std::vector<std::shared_ptr<IHealthInfoCallback>> callbacks_;
};

// AIDL implementation of HIDL_FETCH_IHealth.
// Passthrough implementation of the health service. Use default configuration.
// It does not invoke callbacks unless update() is called explicitly. No
// background thread is spawned to handle callbacks.
//
// The passthrough implementation is only allowed in recovery mode, charger, and
// opened by the binder service.
// If Android is booted normally, the binder service is used instead.
std::shared_ptr<IHealth> GetDefaultPassthroughHealth();

}  // namespace health
}  // namespace hardware
}  // namespace android
}  // namespace aidl
