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

#include <android-base/unique_fd.h>
#include <healthd/healthd.h>
#include <health2impl/HalHealthLoop.h>

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

// An implementation of HealthLoop for health HAL implementation. Specifically,
// it handles binder events, and notifies the given Health implementation to
// send callbacks periodically.
class BinderHealthLoop : public HalHealthLoop {
   public:
    BinderHealthLoop(Health* service) : HalHealthLoop(service) {}
   protected:
    virtual void Init(struct healthd_config* config) override;
    virtual int PrepareToWait() override;
    // A subclass may override this if it wants to handle binder events differently.
    virtual void BinderEvent(uint32_t epevents);
   private:
    android::base::unique_fd binder_fd_;
};


}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
