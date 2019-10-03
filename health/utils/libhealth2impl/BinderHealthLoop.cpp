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

#include <health2impl/BinderHealthLoop.h>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

#include <health2impl/Health.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::IPCThreadState;
using android::hardware::setupTransportPolling;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {
void BinderHealthLoop::BinderEvent(uint32_t /*epevents*/) {
    if (binder_fd_ >= 0) {
        handleTransportPoll(binder_fd_);
    }
}

void BinderHealthLoop::Init(struct healthd_config* config) {
    LOG(INFO) << instance_name() << " instance initializing with healthd_config...";

    binder_fd_.reset(setupTransportPolling());

    if (binder_fd_ >= 0) {
        auto binder_event = [](auto* health_loop, uint32_t epevents) {
            static_cast<BinderHealthLoop*>(health_loop)->BinderEvent(epevents);
        };
        if (!RegisterEvent(binder_fd_, binder_event, EVENT_NO_WAKEUP_FD)) {
            PLOG(ERROR) << instance_name() << " instance: Register for binder events failed";
        }
    }

    HalHealthLoop::Init(config);
}

int BinderHealthLoop::PrepareToWait(void) {
    IPCThreadState::self()->flushCommands();
    return HalHealthLoop::PrepareToWait();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
