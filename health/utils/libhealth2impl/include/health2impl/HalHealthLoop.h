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

#pragma once

#include <health/HealthLoop.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

class Health;

// An implementation of HealthLoop for health HAL implementation. Specifically,
// it notifies the given Health implementation to send callbacks periodically.
class HalHealthLoop : public HealthLoop {
   public:
    HalHealthLoop(Health* service) : service_(service) {}
   protected:
    virtual void Init(struct healthd_config* config) override;
    virtual void Heartbeat() override;
    virtual int PrepareToWait() override;
    virtual void ScheduleBatteryUpdate() override;

    const std::string& instance_name();
   private:
    Health* service_;
};


}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
