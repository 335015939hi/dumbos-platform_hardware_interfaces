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

#include "android/hardware/memtrack/translate-ndk.h"

namespace aidl::android::hardware::memtrack::h2a {

// Check Memtrack Flags
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SMAPS_ACCOUNTED) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SMAPS_ACCOUNTED));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SMAPS_UNACCOUNTED) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SMAPS_UNACCOUNTED));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SHARED) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SHARED));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SHARED_PSS) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SHARED_PSS));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::PRIVATE) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_PRIVATE));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SYSTEM) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SYSTEM));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::DEDICATED) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_DEDICATED));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::NONSECURE) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_NONSECURE));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackFlag::SECURE) ==
              static_cast<uint32_t>(V1_aidl::MemtrackRecord::FLAG_SECURE));

// Check Memtrack Types
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackType::OTHER) ==
              static_cast<uint32_t>(V1_aidl::MemtrackType::OTHER));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackType::GL) ==
              static_cast<uint32_t>(V1_aidl::MemtrackType::GL));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackType::GRAPHICS) ==
              static_cast<uint32_t>(V1_aidl::MemtrackType::GRAPHICS));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackType::MULTIMEDIA) ==
              static_cast<uint32_t>(V1_aidl::MemtrackType::MULTIMEDIA));
static_assert(static_cast<uint32_t>(V1_0_hidl::MemtrackType::CAMERA) ==
              static_cast<uint32_t>(V1_aidl::MemtrackType::CAMERA));

__attribute__((warn_unused_result)) bool translate(const V1_0_hidl::MemtrackRecord& in,
                                                   V1_aidl::MemtrackRecord* out) {
    // Convert uint64_t to int64_t (long in AIDL). AIDL doesn't support unsigned types.
    if (in.sizeInBytes > std::numeric_limits<int64_t>::max() || in.sizeInBytes < 0) {
        return false;
    }
    out->sizeInBytes = static_cast<int64_t>(in.sizeInBytes);

    // It's ok to just assign directly, since this is a bitmap.
    out->flags = in.flags;
    return true;
}

}  // namespace aidl::android::hardware::memtrack::h2a
