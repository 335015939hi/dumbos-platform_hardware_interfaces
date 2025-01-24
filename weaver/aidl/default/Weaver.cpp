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

#include "Weaver.h"

namespace aidl {
namespace android {
namespace hardware {
namespace weaver {

// Methods from ::android::hardware::weaver::IWeaver follow.

::ndk::ScopedAStatus Weaver::getConfig(WeaverConfig* out_config) {
    (void)out_config;
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::read(int32_t in_slotId, const std::vector<uint8_t>& in_key, WeaverReadResponse* out_response) {
<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
    (void)in_slotId;
    (void)in_key;
    (void)out_response;
||||||| BASE

    if (in_slotId > 15 || in_key.size() > 16) {
        *out_response = {0, {}};
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(Weaver::STATUS_FAILED));
    }

    if (slot_array[in_slotId].key != in_key) {
        *out_response = {0, {}};
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(Weaver::STATUS_INCORRECT_KEY));
    }

    *out_response = {0, slot_array[in_slotId].value};

=======
    using ::aidl::android::hardware::weaver::WeaverReadStatus;

    if (in_slotId > 15 || in_key.size() > 16) {
        *out_response = {0, {}, WeaverReadStatus::FAILED};
        return ndk::ScopedAStatus::ok();
    }

    if (slot_array[in_slotId].key != in_key) {
        *out_response = {0, {}, WeaverReadStatus::INCORRECT_KEY};
        return ndk::ScopedAStatus::ok();
    }

    *out_response = {0, slot_array[in_slotId].value, WeaverReadStatus::OK};

>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus Weaver::write(int32_t in_slotId, const std::vector<uint8_t>& in_key, const std::vector<uint8_t>& in_value) {
    (void)in_slotId;
    (void)in_key;
    (void)in_value;
    return ::ndk::ScopedAStatus::ok();
}

} //namespace weaver
} //namespace hardware
} //namespace android
} //namespace aidl
