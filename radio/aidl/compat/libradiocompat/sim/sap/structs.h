/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, eithe r express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <aidl/android/hardware/radio/sim/ISap.h>
#include <android/hardware/radio/1.0/types.h>

namespace android::hardware::radio::compat {

V1_0::SapApduType toHidl(const aidl::android::hardware::radio::sim::SapApduType& sapAdpuType);
V1_0::SapTransferProtocol toHidl(
        const aidl::android::hardware::radio::sim::SapTransferProtocol& sapTransferProtocol);

aidl::android::hardware::radio::sim::SapResultCode toAidl(const V1_0::SapResultCode& sapResultCode);
aidl::android::hardware::radio::sim::SapConnectRsp toAidl(const V1_0::SapConnectRsp& sapConnectRsp);
aidl::android::hardware::radio::sim::SapDisconnectType toAidl(
        const V1_0::SapDisconnectType& sapDisconnectType);
aidl::android::hardware::radio::sim::SapStatus toAidl(const V1_0::SapStatus& sapStatus);

}  // namespace android::hardware::radio::compat
