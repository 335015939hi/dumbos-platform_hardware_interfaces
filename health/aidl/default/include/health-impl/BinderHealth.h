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

#include <aidl/android/hardware/health/IHealth.h>
#include <android-base/unique_fd.h>
#include <health-impl/HalHealthLoop.h>
#include <healthd/healthd.h>

namespace aidl {
namespace android {
namespace hardware {
namespace health {

// AIDL version of android::hardware::health::V2_1::implementation::BinderHealth.
// Binderized health HAL implementation.
class BinderHealth : public HalHealthLoop,
                     public IHealth {
  public:
    // |impl| should be the passthrough implementation.
    BinderHealth(const std::string& name, std::shared_ptr<IHealth> impl);

    ndk::ScopedAStatus registerCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override;
    ndk::ScopedAStatus unregisterCallback(
            const std::shared_ptr<IHealthInfoCallback>& callback) override;
    ndk::ScopedAStatus update() override;

    ndk::ScopedAStatus getCapacity(int32_t* out) override { return service()->getCapacity(out); }
    ndk::ScopedAStatus getChargeCounter(int32_t* out) override {
        return service()->getChargeCounter(out);
    }
    ndk::ScopedAStatus getChargeStatus(BatteryStatus* out) override {
        return service()->getChargeStatus(out);
    }
    ndk::ScopedAStatus getCurrentAverage(int32_t* out) override {
        return service()->getCurrentAverage(out);
    }
    ndk::ScopedAStatus getCurrentNow(int32_t* out) override {
        return service()->getCurrentNow(out);
    }
    ndk::ScopedAStatus getDiskStats(std::vector<DiskStats>* out) override {
        return service()->getDiskStats(out);
    }
    ndk::ScopedAStatus getEnergyCounter(int64_t* out) override {
        return service()->getEnergyCounter(out);
    }
    ndk::ScopedAStatus getHealthConfig(HealthConfig* out) override {
        return service()->getHealthConfig(out);
    }
    ndk::ScopedAStatus getHealthInfo(HealthInfo* out) override {
        return service()->getHealthInfo(out);
    }
    ndk::ScopedAStatus getStorageInfo(std::vector<StorageInfo>* out) override {
        return service()->getStorageInfo(out);
    }
    ndk::ScopedAStatus shouldKeepScreenOn(bool* out) override {
        return service()->shouldKeepScreenOn(out);
    }

    binder_status_t dump(int fd, const char** args, uint32_t num_args) override {
        return service()->dump(fd, args, num_args);
    }

    // HalHealthLoop implementation.
    void OnHealthInfoChanged(const HealthInfo& health_info) override;

  protected:
    void Init(struct healthd_config* config) override;
    int PrepareToWait() override;
    // A subclass may override this if it wants to handle binder events differently.
    virtual void BinderEvent(uint32_t epevents);

  private:
    // cookie is the LinkedCallback object itself.
    // Automatically linkToDeath upon construction, and unlinkToDeath upon destruction,
    // so it is always safe to reinterpret_cast the cookie back to the LinkedCallback object.
    class LinkedCallback {
      public:
        LinkedCallback(BinderHealth* health, std::shared_ptr<IHealthInfoCallback> callback);
        ~LinkedCallback();
        inline std::shared_ptr<IHealthInfoCallback> get() { return callback_; }
      private:
        friend class BinderHealth;
        BinderHealth* health_;
        std::shared_ptr<IHealthInfoCallback> callback_;
    };
    friend class BinderHealth::LinkedCallback;

    bool unregisterCallbackInternal(const std::shared_ptr<IHealthInfoCallback>& callback);

    static void serviceDied(void* cookie);

    ndk::ScopedAIBinder_DeathRecipient death_recipient_;
    int binder_fd_ = -1;
    std::mutex callbacks_lock_;
    std::vector<LinkedCallback> callbacks_;
};

}  // namespace health
}  // namespace hardware
}  // namespace android
}  // namespace aidl
