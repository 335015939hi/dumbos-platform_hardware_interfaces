/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <aidl/android/hardware/bluetooth/audio/IBluetoothAudioProviderFactory.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <future>

using aidl::android::hardware::bluetooth::audio::IBluetoothAudioProviderFactory;
using android::ProcessState;
using android::String16;
using ndk::ScopedAStatus;
using ndk::SpAIBinder;

class BluetoothAudioProviderFactoryAidl
    : public testing::TestWithParam<std::string> {
 public:
  virtual void SetUp() override {
    factory_ = IBluetoothAudioProviderFactory::fromBinder(
        SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
    ASSERT_NE(factory_, nullptr);
  }
  std::shared_ptr<IBluetoothAudioProviderFactory> factory_;
};

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(
    BluetoothAudioProviderFactoryAidl);
INSTANTIATE_TEST_SUITE_P(PerInstance, BluetoothAudioProviderFactoryAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(
                             IBluetoothAudioProviderFactory::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ABinderProcess_setThreadPoolMaxThreadCount(1);
  ABinderProcess_startThreadPool();
  return RUN_ALL_TESTS();
}
