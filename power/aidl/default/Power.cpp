/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "Power.h"

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace power {
namespace impl {
namespace example {

ndk::ScopedAStatus Power::setMode(Mode type, bool enabled) {
    LOG(VERBOSE) << "Power setMode: " << static_cast<int32_t>(type) << " to: " << enabled;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isModeSupported(Mode type, bool* _aidl_return) {
    LOG(INFO) << "Power isModeSupported: " << static_cast<int32_t>(type);
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setBoost(Boost type, int32_t durationMs) {
    LOG(VERBOSE) << "Power setBoost: " << static_cast<int32_t>(type)
                 << ", duration: " << durationMs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isBoostSupported(Boost type, bool* _aidl_return) {
    LOG(INFO) << "Power isBoostSupported: " << static_cast<int32_t>(type);
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::getFixedPerformanceScaleFactor(int32_t* _aidl_return) {
    LOG(INFO) << "Power getFixedPerformanceScaleFactor";
    *_aidl_return = 0;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::isHintSupported(Hint hint, bool* _aidl_return) {
    LOG(INFO) << "Power isHintSupported: " << static_cast<int32_t>(hint);
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setDisplayUpdateImminent(int64_t targetNs) {
    LOG(VERBOSE) << "Power setDisplayUpdateImminent: " << targetNs;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setTopAppPackageName(const std::string& packageName) {
    LOG(VERBOSE) << "Power setTopAppPackageName: " << packageName;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setRenderingRate(int32_t renderingRate) {
    LOG(VERBOSE) << "Power setRenderingRate: " << renderingRate;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::notifyLoadChanged(float cpuChange, float gpuChange) {
    LOG(VERBOSE) << "Power notifyLoadChanged: CPU " << cpuChange << ", GPU " << gpuChange;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Power::setThreadHint(int thread, ThreadHint hint, bool enabled) {
    LOG(VERBOSE) << "Power setThreadHint: thread " << thread << ", hint "
                 << static_cast<int32_t>(hint) << ", enabled " << enabled;
    return ndk::ScopedAStatus::ok();
}

}  // namespace example
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace android
}  // namespace aidl
