/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_HEALTH_FILESYSTEM_V1_0_FILESYSTEM_H
#define ANDROID_HARDWARE_HEALTH_FILESYSTEM_V1_0_FILESYSTEM_H

#include <android/hardware/health/filesystem/1.0/IFileSystem.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace health {
namespace filesystem {
namespace V1_0 {
namespace implementation {

using ::android::sp;
using ::android::hardware::Return;

struct FileSystem : public IFileSystem {
    // Methods from ::android::hardware::health::filesystem::V1_0::IFileSystem follow.
    Return<Result> manualGarbageCollect() override;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace filesystem
}  // namespace health
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_HEALTH_FILESYSTEM_V1_0_FILESYSTEM_H
