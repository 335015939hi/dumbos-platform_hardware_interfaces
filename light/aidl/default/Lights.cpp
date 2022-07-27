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

#include "Lights.h"

#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

int numDefaultLights = 3;

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    LOG(INFO) << "Lights setting state for id=" << id << " to color " << std::hex << state.color;
    if (id < 0 || id > numDefaultLights) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    } else {
        return ndk::ScopedAStatus::ok();
    }
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    for (unsigned int i = 1; i <= numDefaultLights; i++) {
        HwLight light;
        light.id = light.ordinal = i;
        lights->push_back(light);
    }
    LOG(INFO) << "Lights reporting supported lights";
    return ndk::ScopedAStatus::ok();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
