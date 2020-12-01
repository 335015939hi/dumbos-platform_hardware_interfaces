/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "android/hardware/memtrack/translate-cpp.h"

namespace android::h2a {

__attribute__((warn_unused_result)) bool translate(
        const ::android::hardware::memtrack::V1_0::MemtrackRecord& in,
        android::hardware::memtrack::MemtrackRecord* out) {
    // Convert uint64_t to int64_t (long in AIDL). AIDL doesn't support unsinged types.
    if (in.sizeInBytes > 9223372036854775807L || in.sizeInBytes < 0) {
        return false;
    }
    out->sizeInBytes = in.sizeInBytes;

    // Convert uint32_t to int32_t (int in AIDL)
    // It's ok to just assign directly, since this is a bitmap.
    out->flags = in.flags;
    return true;
}

}  // namespace android::h2a
