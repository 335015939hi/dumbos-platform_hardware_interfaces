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

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;

namespace {
constexpr char kFakeSsid[] = "FakeSsid";
constexpr char kFakePsk[] = "FakePsk123";
constexpr uint8_t kFakeWepKey[] = {0x56, 0x67, 0x67, 0xf4, 0x56};
constexpr uint32_t kFakeKeyMgmt = (ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK |
                                   ISupplicantStaNetwork::KeyMgmtMask::WPA_EAP);
constexpr uint32_t kFakeProto = (ISupplicantStaNetwork::ProtoMask::OSEN |
                                 ISupplicantStaNetwork::ProtoMask::RSN);
constexpr uint32_t kFakeAuthAlg = (ISupplicantStaNetwork::AuthAlgMask::OPEN |
                                   ISupplicantStaNetwork::AuthAlgMask::SHARED);
constexpr uint32_t kFakeGroupCipher =
    (ISupplicantStaNetwork::GroupCipherMask::CCMP |
     ISupplicantStaNetwork::GroupCipherMask::WEP104);
constexpr uint32_t kFakePairwiseCipher =
    (ISupplicantStaNetwork::PairwiseCipherMask::CCMP |
     ISupplicantStaNetwork::PairwiseCipherMask::TKIP);
}  // namespace

class SupplicantStaNetworkHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    startSupplicantAndWaitForHidlService();
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
  startSupplicantAndWaitForHidlService();
  EXPECT_NE(getSupplicantStaNetwork().get(), nullptr);
  stopSupplicant();
}

/*
 * SetGet
 * Tests out the various setter/getter methods.
 */
TEST_F(SupplicantStaNetworkHidlTest, SetGet) {
  std::vector<uint8_t> set_ssid(std::begin(kFakeSsid), std::begin(kFakeSsid));
  sta_network_->setSsid(set_ssid, [](const SupplicantStatus& status) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
  });
  sta_network_->getSsid(
      [&](const SupplicantStatus& status, const hidl_vec<uint8_t>& get_ssid) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(set_ssid, std::vector<uint8_t>(get_ssid));
      });

  sta_network_->setKeyMgmt(kFakeKeyMgmt, [](const SupplicantStatus& status) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
  });
  sta_network_->getKeyMgmt(
      [&](const SupplicantStatus& status, uint32_t key_mgmt) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(key_mgmt, kFakeKeyMgmt);
      });

  sta_network_->setProto(kFakeProto, [](const SupplicantStatus& status) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
  });
  sta_network_->getProto([&](const SupplicantStatus& status, uint32_t proto) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
    EXPECT_EQ(proto, kFakeProto);
  });

  sta_network_->setAuthAlg(kFakeAuthAlg, [](const SupplicantStatus& status) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
  });
  sta_network_->getAuthAlg(
      [&](const SupplicantStatus& status, uint32_t auth_alg) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(auth_alg, kFakeAuthAlg);
      });

  sta_network_->setGroupCipher(
      kFakeGroupCipher, [](const SupplicantStatus& status) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
      });
  sta_network_->getGroupCipher(
      [&](const SupplicantStatus& status, uint32_t group_cipher) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(group_cipher, kFakeGroupCipher);
      });

  sta_network_->setPairwiseCipher(
      kFakePairwiseCipher, [](const SupplicantStatus& status) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
      });
  sta_network_->getPairwiseCipher(
      [&](const SupplicantStatus& status, uint32_t pairwise_cipher) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(pairwise_cipher, kFakePairwiseCipher);
      });

  sta_network_->setPskPassphrase(kFakePsk, [](const SupplicantStatus& status) {
    EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
  });
  sta_network_->getPskPassphrase(
      [&](const SupplicantStatus& status, const hidl_string& psk) {
        EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
        EXPECT_EQ(kFakePsk, std::string(psk.c_str()));
      });

  for (uint32_t i = 0;
       i < static_cast<uint32_t>(
               ISupplicantStaNetwork::ParamSizeLimits::WEP_KEYS_MAX_NUM);
       i++) {
    std::vector<uint8_t> set_wep_key(std::begin(kFakeWepKey),
                                     std::end(kFakeWepKey));
    sta_network_->setWepKey(i, set_wep_key, [](const SupplicantStatus& status) {
      EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
    });
    sta_network_->getWepKey(
        i,
        [&](const SupplicantStatus& status,
            const hidl_vec<uint8_t>& get_wep_key) {
          EXPECT_EQ(status.code, SupplicantStatusCode::SUCCESS);
          EXPECT_EQ(set_wep_key, std::vector<uint8_t>(get_wep_key));
        });
  }
}
