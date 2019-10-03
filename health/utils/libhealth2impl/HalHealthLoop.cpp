/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <health2impl/HalHealthLoop.h>

#include <android-base/logging.h>
#include <hal_conversion.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

#include <health2impl/Health.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::IPCThreadState;
using android::hardware::setupTransportPolling;

using android::hardware::health::V1_0::hal_conversion::convertFromHealthConfig;
using android::hardware::health::V2_0::Result;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

void HalHealthLoop::Init(struct healthd_config* config) {
    // Retrieve healthd_config from the HAL.
    service_->getHealthConfig([config](auto res, const auto& health_config) {
        CHECK(res == Result::SUCCESS);

        convertFromHealthConfig(health_config.battery, config);
        config->boot_min_cap = health_config.bootMinCap;

        // Leave screen_on empty because it is handled in GetScreenOn below.

        // Leave ignorePowerSupplyNames empty because it isn't
        // used by clients of health HAL.
    });
}

void HalHealthLoop::Heartbeat(void) {
    // noop
}

void HalHealthLoop::ScheduleBatteryUpdate() {
    Result res = service_->update();
    if (res == Result::SUCCESS) {
        return;
    }
    if (res == Result::NOT_SUPPORTED) {
        LOG(ERROR) << "update() on the health HAL implementation is unsupported.";
        return;
    }
    LOG(FATAL) << "update() failed with " << toString(res);
}

int HalHealthLoop::PrepareToWait() {
    return -1;
}

std::optional<bool> HalHealthLoop::GetScreenOn() {
    std::optional<bool> ret;
    service_->getScreenOn([&](auto res, auto screen_on) {
        if (res == Result::SUCCESS) {
            *ret = screen_on;
            return;
        }
        CHECK(res == Result::NOT_SUPPORTED) << "getScreenOn failed with " << toString(res);
    });
    return ret;
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
