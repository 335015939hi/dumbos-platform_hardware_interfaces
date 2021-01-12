/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "health-impl/BinderHealth.h"
#include "health-impl/Health.h"

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::health::BinderHealth;
using aidl::android::hardware::health::GetDefaultPassthroughHealth;

static constexpr const char* gInstanceName = "default";

int main() {
    // make a default health service
    // TODO(b/177269435): should dlopen the passthrough service
    auto passthrough = GetDefaultPassthroughHealth();
    CHECK(passthrough != nullptr)
            << "Cannot find passthrough implementation of health AIDL HAL for instance "
            << gInstanceName;

    auto binder = ndk::SharedRefBase::make<BinderHealth>(gInstanceName, passthrough);
    return binder->StartLoop();
}
