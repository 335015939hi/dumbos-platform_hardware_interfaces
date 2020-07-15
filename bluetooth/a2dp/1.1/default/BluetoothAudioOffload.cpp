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

#include "BluetoothAudioOffload.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace a2dp {
namespace V1_1 {
namespace implementation {

// Methods from ::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioOffload follow.
Return<::android::hardware::bluetooth::a2dp::V1_0::Status> BluetoothAudioOffload::startSession(const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf, const ::android::hardware::bluetooth::a2dp::V1_0::CodecConfiguration& codecConfig) {
    // TODO implement
    return ::android::hardware::bluetooth::a2dp::V1_0::Status {};
}

Return<void> BluetoothAudioOffload::streamStarted(::android::hardware::bluetooth::a2dp::V1_0::Status status) {
    // TODO implement
    return Void();
}

Return<void> BluetoothAudioOffload::streamSuspended(::android::hardware::bluetooth::a2dp::V1_0::Status status) {
    // TODO implement
    return Void();
}

Return<void> BluetoothAudioOffload::endSession() {
    // TODO implement
    return Void();
}


// Methods from ::android::hardware::bluetooth::a2dp::V1_1::IBluetoothAudioOffload follow.
Return<::android::hardware::bluetooth::a2dp::V1_0::Status> BluetoothAudioOffload::startSession_1_1(const sp<::android::hardware::bluetooth::a2dp::V1_0::IBluetoothAudioHost>& hostIf, const ::android::hardware::bluetooth::a2dp::V1_1::CodecConfiguration& codecConfig) {
    // TODO implement
    return ::android::hardware::bluetooth::a2dp::V1_0::Status {};
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

//IBluetoothAudioOffload* HIDL_FETCH_IBluetoothAudioOffload(const char* /* name */) {
    //return new BluetoothAudioOffload();
//}
//
}  // namespace implementation
}  // namespace V1_1
}  // namespace a2dp
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
