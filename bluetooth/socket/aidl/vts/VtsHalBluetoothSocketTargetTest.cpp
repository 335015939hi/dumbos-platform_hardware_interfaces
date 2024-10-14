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
  ScopedAStatus onOpened(int64_t in_socketId) override;
  ScopedAStatus onOpenFailed(int64_t in_socketId,
                             const std::string& in_failureReason) override;
  ScopedAStatus onCloseRequest(int64_t in_socketId,
                               const std::string& in_requestReason) override;
};

ScopedAStatus BluetoothSocketCallback::onOpened(int64_t /*in_socketId*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothSocketCallback::onOpenFailed(
    int64_t /*in_socketId*/, const std::string& /*in_failureReason*/) {
  return ::ndk::ScopedAStatus::ok();
}
ScopedAStatus BluetoothSocketCallback::onCloseRequest(
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
  ScopedAStatus open(SocketContext& in_context);
  ScopedAStatus close(long in_socket_id);

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

ScopedAStatus BluetoothSocketTest::open(SocketContext& in_context) {
  return bluetooth_socket_->open(in_context);
}

ScopedAStatus BluetoothSocketTest::close(long in_socketId) {
  return bluetooth_socket_->close(in_socketId);
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
}

TEST_P(BluetoothSocketTest, OpenSocket) {
  SocketContext socket_context;
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
  status = open(socket_context);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, CloseSocket) {
  long socket_id = 1;
  ScopedAStatus status = registerCallback();
  ASSERT_TRUE(status.isOk());
  status = close(socket_id);
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
