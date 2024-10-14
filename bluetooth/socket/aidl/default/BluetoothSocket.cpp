/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include "BluetoothSocket.h"

namespace aidl::android::hardware::bluetooth::socket::impl {

BluetoothSocket::BluetoothSocket() {}
BluetoothSocket::~BluetoothSocket() {}

::ndk::ScopedAStatus BluetoothSocket::initialize(
    const std::shared_ptr<
        ::aidl::android::hardware::bluetooth::socket::IBluetoothSocketCallback>&
    /*in_callback*/) {
  return ::ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus BluetoothSocket::getSocketCapabilities(
    ::aidl::android::hardware::bluetooth::socket::SocketCapabilities*
    /*_aidl_return*/) {
  return ::ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus BluetoothSocket::open(
    const ::aidl::android::hardware::bluetooth::socket::SocketContext&
    /*in_context*/) {
  return ::ndk::ScopedAStatus::ok();
}
::ndk::ScopedAStatus BluetoothSocket::close(int64_t /*in_socketId*/) {
  return ::ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::bluetooth::socket::impl
