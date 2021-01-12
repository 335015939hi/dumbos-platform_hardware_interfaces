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
#include <android/binder_auto_utils.h>
#include <healthd/BatteryMonitor.h>
#include <healthd/healthd.h>

namespace aidl::android::hardware::health {

// AIDL version of android::hardware::health::V2_1::implementation::Health.
// Sample passthrough implementation of health HAL.
class Health : public BnHealth {
  public:
    // Initialize with |config|.
    // A subclass may modify |config| before passing it to the parent constructor.
    // See implementation of Health for code samples.
    Health(std::unique_ptr<healthd_config>&& config);

    // This class does not handle callbacks, and subclasses of this class must
    // not handle callbacks either. Callbacks should only be handled by BinderHealth.
    ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override final;
    ndk::ScopedAStatus unregisterCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override final;
    ndk::ScopedAStatus update() override final;

    // A subclass should not override this because this is a direct translation of
    // the internal healthd_config to HealthConfig. Modify healthd_config in the constructor
    // instead.
    ndk::ScopedAStatus getHealthConfig(HealthConfig* out) override final;

    // A subclass should not override this. Override UpdateHealthInfo instead.
    ndk::ScopedAStatus getHealthInfo(HealthInfo* out) override final;

    // A subclass is recommended to override the path in healthd_config in the constructor.
    // Only override these if there are no suitable kernel interfaces to read these values.
    ndk::ScopedAStatus getChargeCounter(int32_t* out) override;
    ndk::ScopedAStatus getCurrentNow(int32_t* out) override;
    ndk::ScopedAStatus getCurrentAverage(int32_t* out) override;
    ndk::ScopedAStatus getCapacity(int32_t* out) override;
    ndk::ScopedAStatus getChargeStatus(BatteryStatus* out) override;

    // A subclass may either override these or provide function pointers in
    // in healthd_config the constructor.
    ndk::ScopedAStatus getEnergyCounter(int64_t* out) override;
    ndk::ScopedAStatus shouldKeepScreenOn(bool* out) override;

    // A subclass may override these for a specific device.
    // The default implementations return nothing in |out|.
    ndk::ScopedAStatus getDiskStats(std::vector<DiskStats>* out) override;
    ndk::ScopedAStatus getStorageInfo(std::vector<StorageInfo>* out) override;

    // A subclass may override these to provide a better implementation.
    binder_status_t dump(int fd, const char** args, uint32_t num_args) override;

  protected:
    // A subclass can override this to modify any health info object before
    // returning to clients. This is similar to healthd_board_battery_update().
    // By default, it does nothing.
    // See implementation of Health for code samples.
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

}  // namespace aidl::android::hardware::health
