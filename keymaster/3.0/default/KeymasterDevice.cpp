/*
 **
 ** Copyright 2016, The Android Open Source Project
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **     http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "android.hardware.keymaster@3.0-impl"

#include "KeymasterDevice.h"

#include <log/log.h>

#include <AndroidKeymaster3Device.h>
#include <hardware/keymaster_defs.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V3_0 {
namespace implementation {

static IKeymasterDevice* createKeymaster3Device() {
    return ::keymaster::ng::CreateKeymasterDevice();
}

IKeymasterDevice* HIDL_FETCH_IKeymasterDevice(const char* name) {
    ALOGI("Fetching keymaster device name %s", name);

    if (name && strcmp(name, "softwareonly") == 0) {
        return ::keymaster::ng::CreateKeymasterDevice();
    } else if (name && strcmp(name, "default") == 0) {
        return createKeymaster3Device();
    }
    return nullptr;
}

}  // namespace implementation
}  // namespace V3_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
