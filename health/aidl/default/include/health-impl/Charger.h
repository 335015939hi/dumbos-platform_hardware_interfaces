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

#include <optional>
#include <type_traits>

#include <android/binder_auto_utils.h>
#include <health-impl/Health.h>
#include <healthd_mode_charger.h>

#pragma once

namespace aidl::android::hardware::health::charger {

// Implements ChargerHalHealthLoopInterface for AIDL charger
// Adapter of (Health, HalHealthLoop) -> ChargerHalHealthLoopInterface
class ChargerCallback : public ::android::ChargerHalHealthLoopInterface {
  public:
    ChargerCallback(std::shared_ptr<Health> service) : service_(std::move(service)) {}
    std::optional<bool> ChargerShouldKeepScreenOn() override {
        return service_->ShouldKeepScreenOn();
    }
    bool ChargerIsOnline() override { return hal_health_loop_->charger_online(); }
    void ChargerInitConfig(healthd_config* config, void* cookie) override {
        auto hal_health_loop = reinterpret_cast<HalHealthLoop*>(cookie);
        return service_->OnInit(hal_health_loop, config);
    }
    int ChargerRegisterEvent(int fd, BoundFunction func, EventWakeup wakeup) override {
        return hal_health_loop_->RegisterEvent(fd, func, wakeup);
    }

    void set_hal_health_loop(std::shared_ptr<HalHealthLoop> hal_health_loop) {
        hal_health_loop_ = std::move(hal_health_loop);
    }

  private:
    std::shared_ptr<Health> service_;
    std::shared_ptr<HalHealthLoop> hal_health_loop_;
};

// Implements HalHealthLoopCallback for AIDL charger
// Adapter of (Charger, Health) ->  HalHealthLoopCallback
class LoopCallback : public HalHealthLoopCallback {
  public:
    LoopCallback(std::shared_ptr<Health> service,
                 std::shared_ptr<::android::ChargerHalHealthLoopInterface> charger_callback)
        : service_(std::move(service)),
          charger_callback_(charger_callback),
          charger_(std::make_unique<::android::Charger>(charger_callback.get())) {}

    void OnHeartbeat() override {
        service_->OnHeartbeat();
        charger_->OnHeartbeat();
    }
    int OnPrepareToWait() override {
        int timeout1 = service_->OnPrepareToWait();
        int timeout2 = charger_->OnPrepareToWait();

        if (timeout1 < 0) return timeout2;
        if (timeout2 < 0) return timeout1;
        return std::min(timeout1, timeout2);
    }
    void OnInit(HalHealthLoop* hal_health_loop, struct healthd_config* config) override {
        // Charger::OnInit calls ChargerInitConfig, which calls into the real Health::OnInit.
        charger_->OnInit(config, reinterpret_cast<void*>(hal_health_loop));
    }
    void OnHealthInfoChanged(const HealthInfo& health_info) override {
        charger_->OnHealthInfoChanged(::android::ChargerHealthInfo{
                .battery_level = health_info.batteryLevel,
                .battery_status = health_info.batteryStatus,
        });
        service_->OnHealthInfoChanged(health_info);
    }

  private:
    std::shared_ptr<Health> service_;
    // Ensures the lifetime of |charger_callback_| is longer than |charger_|.
    std::shared_ptr<::android::ChargerHalHealthLoopInterface> charger_callback_;
    std::unique_ptr<::android::Charger> charger_;
};

}  // namespace aidl::android::hardware::health::charger
