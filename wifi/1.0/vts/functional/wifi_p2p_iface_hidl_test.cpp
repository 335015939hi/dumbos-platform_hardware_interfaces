/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the P2pache License, Version 2.0 (the "License");
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

#include <android-base/logging.h>

#include <android/hardware/wifi/1.0/IWifiP2pIface.h>

#include <gtest/gtest.h>

#include "wifi_hidl_test_utils.h"

using ::android::hardware::wifi::V1_0::IWifiP2pIface;
using ::android::sp;

class WifiP2pIfaceHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    stopFramework();

    wifi_p2p_iface_ = getWifiP2pIface();
    ASSERT_NE(wifi_p2p_iface_, nullptr);
  }

  virtual void TearDown() override { startFramework(); }

 protected:
  sp<IWifiP2pIface> wifi_p2p_iface_;
};

/*
 * Create:
 * Ensures that an instance of the IWifiP2pIface proxy object is
 * successfully created.
 */
TEST_F(WifiP2pIfaceHidlTest, Create) {}
