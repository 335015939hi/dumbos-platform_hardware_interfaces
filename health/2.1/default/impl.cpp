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

#include <string_view>
#include <health2impl/Health.h>

using android::hardware::health::V2_1::IHealth;
using android::hardware::health::V2_1::implementation::Health;

using namespace std::literals;

// Passthrough implementation of the health service. Use default configuration.
// A new thread will be started to handle callbacks.
// The passthrough implementation is only allowed in recovery and charger mode.
// If Android is booted normally, the hwbinder service is used instead.
extern "C" IHealth* HIDL_FETCH_IHealth(const char* instance) {
    // This implementation only implements the "default" instance. It rejects
    // other instance names.
    // Note that the Android framework only reads values from the "default"
    // health HAL 2.1 instance.
    if (instance != "default"sv) {
        return nullptr;
    }

    // Note that HealthLoop cannot be destroyed. Hence, always return the same
    // instance.
    static android::sp<IHealth> default_instance = new Health(instance);

    return default_instance.get();
}
