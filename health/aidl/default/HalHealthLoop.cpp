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

#include <health-impl/HalHealthLoop.h>

#include <android-base/logging.h>

#include <health-impl/Health.h>
#include <health-impl/health-convert.h>

namespace aidl::android::hardware::health {

void HalHealthLoop::Init(struct healthd_config* config) {
    // Retrieve healthd_config from the HAL.
    HealthConfig health_config;
    auto res = service_->getHealthConfig(&health_config);
    CHECK(res.isOk());
    convert(health_config, config);
    // Leave screen_on empty because clients call Health::getScreenOn directly.
    // Leave ignorePowerSupplyNames empty because it isn't
    // used by clients of health HAL.
}

void HalHealthLoop::Heartbeat(void) {
    // noop
}

void HalHealthLoop::ScheduleBatteryUpdate() {
    // ignore errors. impl may not be able to handle any callbacks, so
    // update() may return errors.
    if (auto res = service_->update(); !res.isOk()) {
        LOG(WARNING) << "update() on the health HAL implementation failed with "
                     << res.getDescription();
    }

    HealthInfo health_info;
    auto res = service_->getHealthInfo(&health_info);
    CHECK(res.isOk()) << "getHealthInfo() on the health HAL implementation failed with "
                      << res.getDescription();
    this->OnHealthInfoChanged(health_info);
}

int HalHealthLoop::PrepareToWait() {
    return -1;
}

void HalHealthLoop::OnHealthInfoChanged(const HealthInfo& health_info) {
    set_charger_online(health_info);
    AdjustWakealarmPeriods(charger_online());
}

void HalHealthLoop::set_charger_online(const HealthInfo& health_info) {
    charger_online_ = health_info.chargerAcOnline || health_info.chargerUsbOnline ||
                      health_info.chargerWirelessOnline;
}

}  // namespace aidl::android::hardware::health
