/*
 * Copyright (C) 2024 The Android Open Source Project
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

#pragma once

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <aidl/android/hardware/sharedmem/BpSharedMem.h>

namespace aidl {
namespace android {
namespace hardware {
namespace sharedmem {

class SharedMem : public BpSharedMem {
    ndk::ScopedAStatus createRegion(const std::string& in_name, int32_t in_size,
                                    ::ndk::ScopedFileDescriptor* _aidl_return) override;
};

}  // namespace sharedmem
}  // namespace hardware
}  // namespace android
}  // namespace aidl
