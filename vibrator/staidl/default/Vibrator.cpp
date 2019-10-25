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

#include "Vibrator.h"

#include <android-base/logging.h>
#include <thread>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    LOG(INFO) << "Vibrator reporting capabilities";
    *_aidl_return = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK |
                    IVibrator::CAP_AMPLITUDE_CONTROL | IVibrator::CAP_EXTERNAL_CONTROL;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    LOG(INFO) << "Vibrator off";
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    LOG(INFO) << "Vibrator on for timeoutMs: " << timeoutMs;
    if (callback != nullptr) {
        std::thread([=] {
            usleep(timeoutMs);
            callback->onComplete();
        }).detach();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    LOG(INFO) << "Vibrator perform";

    (void)effect;
    (void)strength;

    constexpr size_t kEffectMillis = 100;

    if (callback != nullptr) {
        std::thread([=] {
            usleep(kEffectMillis);
            callback->onComplete();
        }).detach();
    }

    *_aidl_return = kEffectMillis;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(int8_t amplitude) {
    LOG(INFO) << "Vibrator set amplitude: " << amplitude;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    LOG(INFO) << "Vibrator set external control: " << enabled;
    return ndk::ScopedAStatus::ok();
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
