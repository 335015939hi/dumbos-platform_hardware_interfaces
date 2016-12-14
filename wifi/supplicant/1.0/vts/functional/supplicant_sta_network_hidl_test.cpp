/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include <gtest/gtest.h>

#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>

#include "supplicant_hidl_test_utils.h"

using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::sp;

class SupplicantStaNetworkHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    startSupplicant();
    EXPECT_TRUE(turnOnExcessiveLogging());
    sta_network_ = getSupplicantStaNetwork();
    EXPECT_NE(sta_network_.get(), nullptr);
  }

  virtual void TearDown() override { stopSupplicant(); }

 protected:
  sp<ISupplicantStaNetwork> sta_network_;
};

/*
 * Create:
 * Ensures that an instance of the ISupplicantStaNetwork proxy object is
 * successfully created.
 */
TEST(SupplicantStaNetworkHidlTestNoFixture, Create) {
  startSupplicant();
  EXPECT_NE(getSupplicantStaNetwork().get(), nullptr);
  stopSupplicant();
}
