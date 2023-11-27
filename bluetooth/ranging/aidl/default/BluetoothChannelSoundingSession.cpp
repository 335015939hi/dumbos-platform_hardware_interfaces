/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "BluetoothChannelSoundingSession.h"

namespace aidl::android::hardware::bluetooth::ranging::impl {
ndk::ScopedAStatus BluetoothChannelSoundingSession::getVendorSpecificReplies(
    std::optional<
        std::vector<std::optional<VendorSpecificData>>>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::getSupportedResultTypes(
    std::vector<ResultType>* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::isAbortedProcedureRequired(
    bool* /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::writeRawData(
    const ChannelSoudingRawData& /*in_rawData*/) {
  return ::ndk::ScopedAStatus::ok();
}
ndk::ScopedAStatus BluetoothChannelSoundingSession::close(
    Reason /*in_reason*/) {
  return ::ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::hardware::bluetooth::ranging::impl
