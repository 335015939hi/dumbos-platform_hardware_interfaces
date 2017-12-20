/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.combinedhals-service"

#include <android/hardware/health/1.0/IHealth.h>
#include <android/hardware/light/2.0/ILight.h>
#include <android/hardware/memtrack/1.0/IMemtrack.h>
#include <android/hardware/vibrator/1.0/IVibrator.h>
#include <hidl/LegacySupport.h>

using android::hardware::light::V2_0::ILight;
using android::hardware::vibrator::V1_0::IVibrator;
using android::hardware::health::V1_0::IHealth;
using android::hardware::memtrack::V1_0::IMemtrack;

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::registerPassthroughServiceImplementation;

int main() {
    configureRpcThreadpool(1, true /* callerWillJoin */);
    android::status_t status = registerPassthroughServiceImplementation<ILight>();
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering light service: %d", status);

    status = registerPassthroughServiceImplementation<IVibrator>();
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering vibrator service: %d",
                        status);

    status = registerPassthroughServiceImplementation<IHealth>();
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering health service: %d",
                        status);

    status = registerPassthroughServiceImplementation<IMemtrack>();
    LOG_ALWAYS_FATAL_IF(status != android::OK, "Error while registering memtrack service: %d",
                        status);
    joinRpcThreadpool();
}
