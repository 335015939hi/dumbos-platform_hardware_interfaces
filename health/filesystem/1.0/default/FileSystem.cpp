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

#include "FileSystem.h"

#include <hidl/HidlBinderSupport.h>

namespace android {
namespace hardware {
namespace health {
namespace filesystem {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::health::filesystem::V1_0::IFileSystem follow.
Return<Result> FileSystem::garbageCollect() {
    // Exit when garbage collection is completed since this is a lazy HAL.
    addPostCommandTask([]() { exit(0); });
    // The reference implementation does not do anything.
    return Result::SUCCESS;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace filesystem
}  // namespace health
}  // namespace hardware
}  // namespace android
