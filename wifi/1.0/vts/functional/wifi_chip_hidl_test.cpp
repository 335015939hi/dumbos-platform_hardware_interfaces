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

#include <android/hardware/wifi/1.0/IWifiChip.h>

#include <gtest/gtest.h>

#include "wifi_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::V1_0::ChipId;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::wifi::V1_0::IWifiChip;

class WifiChipHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    wifi_chip_ = getWifiChip();
    ASSERT_NE(wifi_chip_, nullptr);
  }

  virtual void TearDown() override { stopWifi(); }

  // Helper function to configure the Chip in one of the supported modes.
  // Most of the non mode configuration related methods require chip
  // to be first configured.
  void configureChip() {
    std::vector<IWifiChip::ChipMode> chip_modes;
    wifi_chip_->getAvailableModes([&](
        const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
      EXPECT_GT(modes.size(), 0u);
      chip_modes = modes;
    });
    wifi_chip_->configureChip(chip_modes[0].id, [&](const WifiStatus& status) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    });
  }

 protected:
  sp<IWifiChip> wifi_chip_;
};

/*
 * Create:
 * Ensures that an instance of the IWifiChip proxy object is
 * successfully created.
 */
TEST(WifiChipHidlTestNoFixture, Create) {
  EXPECT_NE(getWifiChip(), nullptr);
  stopWifi();
}

/*
 * GetId:
 * Invokes the |IWifiChip.getId| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetId) {
  wifi_chip_->getId([&](const WifiStatus& status, ChipId id) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
  });
}

/*
 * GetAvailableMode:
 * Invokes the |IWifiChip.getAvailableModes| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetAvailableModes) {
  wifi_chip_->getAvailableModes([&](
      const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_GT(modes.size(), 0u);
  });
}

/*
 * ConfigureChip:
 * Invokes the |IWifiChip.configureChip| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, ConfigureChip) {
  std::vector<IWifiChip::ChipMode> chip_modes;
  wifi_chip_->getAvailableModes([&](
      const WifiStatus& status, const hidl_vec<IWifiChip::ChipMode>& modes) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_GT(modes.size(), 0u);
    chip_modes = modes;
  });
  for (const auto& mode : chip_modes) {
    wifi_chip_->configureChip(mode.id, [&](const WifiStatus& status) {
      EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    });
  }
}

/*
 * GetCapabilities:
 * Invokes the |IWifiChip.getCapabilities| method and ensures that it
 * succeeds.
 */
TEST_F(WifiChipHidlTest, GetCapabilities) {
  configureChip();
  wifi_chip_->getCapabilities([&](const WifiStatus& status, uint32_t caps) {
    EXPECT_EQ(status.code, WifiStatusCode::SUCCESS);
    EXPECT_NE(caps, 0u);
  });
}
