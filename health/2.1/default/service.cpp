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
#define LOG_TAG "android.hardware.health@2.1-service"

#include <future>

#include <android-base/logging.h>
#include <android/hardware/health/2.1/IHealth.h>
#include <hidl/LegacySupport.h>

using android::sp;
using android::hardware::defaultPassthroughServiceImplementation;
using IHealth_2_1 = ::android::hardware::health::V2_1::IHealth;
using IHealth_2_0 = ::android::hardware::health::V2_0::IHealth;

static void CheckHealthVersion() {
    sp<IHealth_2_0> service_2_0 = IHealth_2_0::getService();
    CHECK(service_2_0) << "V2_0::IHealth::getService() returns nullptr";
    sp<IHealth_2_1> service_2_1 = IHealth_2_1::castFrom(service_2_0);
    CHECK(service_2_1) << "Cannot cast 2.0 health service to 2.1";
}

int main(int /* argc */, char* /* argv */[]) {
    auto _ = std::async(&CheckHealthVersion);
    return defaultPassthroughServiceImplementation<IHealth_2_0>();
}
