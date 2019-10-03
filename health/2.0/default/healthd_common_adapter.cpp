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

// Support legacy functions in healthd/healthd.h using healthd_mode_ops.
// New code should use HealthLoop directly instead.

#include <memory>

#include <cutils/klog.h>
#include <health/HealthLoop.h>
#include <health2/Health.h>
#include <healthd/healthd.h>

using android::hardware::health::HealthLoop;
using android::hardware::health::V2_0::implementation::Health;

static std::unique_ptr<HealthLoop> health_loop;
struct healthd_mode_ops* healthd_mode_ops = nullptr;

// Adapter of HealthLoop to use legacy healthd_mode_ops.
class HealthLoopAdapter : public HealthLoop {
    void Init(healthd_config* config) override { healthd_mode_ops->init(config); }
    void Heartbeat() override { healthd_mode_ops->heartbeat(); }
    int PrepareToWait() override { return healthd_mode_ops->preparetowait(); }
    void ScheduleBatteryUpdate() override { Health::getImplementation()->update(); }
};

int healthd_register_event(int fd, void (*handler)(uint32_t), EventWakeup wakeup) {
    auto wrapped_handler = [handler](auto*, uint32_t epevents) { handler(epevents); };
    return health_loop->RegisterEvent(fd, wrapped_handler, wakeup);
}

void healthd_battery_update_internal(bool charger_online) {
    health_loop->SetChargerOnline(charger_online);
}

int healthd_main() {
    if (!healthd_mode_ops) {
        KLOG_ERROR("healthd ops not set, exiting\n");
        exit(1);
    }

    health_loop = std::make_unique<HealthLoopAdapter>();

    int ret = health_loop->StartLoop();

    // Should not reach here. The following will exit().
    health_loop.reset();

    return ret;
}
