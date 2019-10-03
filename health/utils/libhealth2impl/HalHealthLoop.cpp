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

void HalHealthLoop::Init(struct healthd_config* config) {
    service_->Init(this, config);
}

void HalHealthLoop::Heartbeat(void) {
    // noop
}

void HalHealthLoop::ScheduleBatteryUpdate() {
    service_->ScheduleBatteryUpdate();
}

const std::string& HalHealthLoop::instance_name() {
    return service_->instance_name();
}

int HalHealthLoop::PrepareToWait() {
    return -1;
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
