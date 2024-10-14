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
using ::aidl::android::hardware::bluetooth::socket::SocketContext;
using ::aidl::android::hardware::bluetooth::socket::SocketEvent;
using ::aidl::android::hardware::bluetooth::socket::SocketProperties;
using ::ndk::ScopedAStatus;

class BluetoothSocketCallback : public BnBluetoothSocketCallback {
  ScopedAStatus onSocketEventReceived(
      const SocketEvent& inSocketEvent) override;
};

ScopedAStatus BluetoothSocketCallback::onSocketEventReceived(
    const SocketEvent& /*inSocketEvent*/) {
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

  ScopedAStatus initialize();
  ScopedAStatus getSocketProperties(
      std::optional<std::vector<std::optional<SocketProperties>>>*
          _aidl_return);
  ScopedAStatus notifySocketConnectionStateChange(SocketContext& in_context);

 private:
  std::shared_ptr<IBluetoothSocket> bluetooth_socket_;
};

ScopedAStatus BluetoothSocketTest::initialize() {
  std::shared_ptr<BluetoothSocketCallback> callback = nullptr;
  callback = ndk::SharedRefBase::make<BluetoothSocketCallback>();
  return bluetooth_socket_->initialize(callback);
}

ScopedAStatus BluetoothSocketTest::getSocketProperties(
    std::optional<std::vector<std::optional<SocketProperties>>>* _aidl_return) {
  return bluetooth_socket_->getSocketProperties(_aidl_return);
}

ScopedAStatus BluetoothSocketTest::notifySocketConnectionStateChange(
    SocketContext& in_context) {
  return bluetooth_socket_->notifySocketConnectionStateChange(in_context);
}

TEST_P(BluetoothSocketTest, Initialize) {
  ScopedAStatus status = initialize();
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, GetSocketProperties) {
  std::optional<std::vector<std::optional<SocketProperties>>> socket_properties;
  ScopedAStatus status = initialize();
  ASSERT_TRUE(status.isOk());
  status = getSocketProperties(&socket_properties);
  ASSERT_TRUE(status.isOk());
}

TEST_P(BluetoothSocketTest, NotifySocketConnectionStateChange) {
  SocketContext socket_context;
  ScopedAStatus status = initialize();
  ASSERT_TRUE(status.isOk());
  status = notifySocketConnectionStateChange(socket_context);
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
