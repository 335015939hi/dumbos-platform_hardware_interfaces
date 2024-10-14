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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/socket/BnBluetoothSocketCallback.h>
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocket.h>
#include <aidl/android/hardware/bluetooth/socket/IBluetoothSocketCallback.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

using ::aidl::android::hardware::bluetooth::socket::BnBluetoothSocketCallback;
using ::aidl::android::hardware::bluetooth::socket::IBluetoothSocket;
using ::aidl::android::hardware::bluetooth::socket::SocketCapabilities;
using ::aidl::android::hardware::bluetooth::socket::SocketContext;
using ::ndk::ScopedAStatus;

class BluetoothSocketCallback : public BnBluetoothSocketCallback {
  ScopedAStatus openedComplete(
      int64_t in_socketId,
      ::aidl::android::hardware::bluetooth::socket::Status in_status,
      const std::string& in_reason) override;
  ScopedAStatus close(int64_t in_socketId,
                      const std::string& in_reason) override;
};

ScopedAStatus BluetoothSocketCallback::openedComplete(
    int64_t /*in_socketId*/,
    ::aidl::android::hardware::bluetooth::socket::Status /*in_status*/,
    const std::string& /*in_reason*/) {
  return ::ndk::ScopedAStatus::ok();
}

ScopedAStatus BluetoothSocketCallback::close(
    int64_t /*in_socketId*/, const std::string& /*in_requestReason*/) {
  return ::ndk::ScopedAStatus::ok();
}

class BluetoothSocketTest : public ::testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    ALOGI("SetUp Socket Test");
    bluetooth_socket_ = IBluetoothSocket::fromBinder(
        ndk::SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(bluetooth_socket_, nullptr);
  }

  virtual void TearDown() override {
    ALOGI("TearDown Socket Test");
    bluetooth_socket_ = nullptr;
    ASSERT_EQ(bluetooth_socket_, nullptr);
  }

  ScopedAStatus registerCallback();
  ScopedAStatus getSocketCapabilities(SocketCapabilities* _aidl_return);
  ScopedAStatus opened(SocketContext& in_context);
  ScopedAStatus closed(long in_socket_id);

 private:
  std::shared_ptr<IBluetoothSocket> bluetooth_socket_;
};

ScopedAStatus BluetoothSocketTest::registerCallback() {
  std::shared_ptr<BluetoothSocketCallback> callback = nullptr;
  callback = ndk::SharedRefBase::make<BluetoothSocketCallback>();
  return bluetooth_socket_->registerCallback(callback);
}

ScopedAStatus BluetoothSocketTest::getSocketCapabilities(
    SocketCapabilities* _aidl_return) {
  return bluetooth_socket_->getSocketCapabilities(_aidl_return);
}

ScopedAStatus BluetoothSocketTest::opened(SocketContext& in_context) {
  return bluetooth_socket_->opened(in_context);
}

ScopedAStatus BluetoothSocketTest::closed(long in_socketId) {
  return bluetooth_socket_->closed(in_socketId);
}

TEST_P(BluetoothSocketTest, registerCallback) {
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, GetSocketCapabilities) {
  SocketCapabilities socket_capabilities;
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
  status = getSocketCapabilities(&socket_capabilities);
  ASSERT_TRUE(status.isOk());
  ASSERT_TRUE(socket_capabilities.leCocCapabilities.numberOfSupportedSockets >=
              0);
  if (socket_capabilities.leCocCapabilities.numberOfSupportedSockets) {
    ASSERT_TRUE(socket_capabilities.leCocCapabilities.mtu >= 25 &&
                socket_capabilities.leCocCapabilities.mtu <= 65535);
  }
}

TEST_P(BluetoothSocketTest, Opened) {
  SocketContext socket_context;
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
  status = opened(socket_context);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, Closed) {
  long socket_id = 1;
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
  status = closed(socket_id);
  ASSERT_TRUE(status.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(BluetoothSocketTest);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothSocketTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothSocket::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_startThreadPool();
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
