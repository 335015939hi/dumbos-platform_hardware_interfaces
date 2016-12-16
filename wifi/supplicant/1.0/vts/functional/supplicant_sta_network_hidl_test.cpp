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

#include <VtsHalHidlTargetBaseTest.h>

#include <android/hardware/wifi/supplicant/1.0/ISupplicantStaNetwork.h>

#include "supplicant_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;

namespace {
constexpr char kTestSsidStr[] = "TestSsid1234";
constexpr char kTestPsk[] = "TestPsk123";
constexpr char kTestEapPasswdStr[] = "TestEapPasswd1234";
constexpr uint8_t kTestBssid[] = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
constexpr uint8_t kTestWepKey[] = {0x56, 0x67, 0x67, 0xf4, 0x56};
constexpr uint8_t kTestKc[] = {0x56, 0x67, 0x67, 0xf4, 0x76, 0x87, 0x98, 0x12};
constexpr uint8_t kTestSres[] = {0x56, 0x67, 0x67, 0xf4};
constexpr uint8_t kTestRes[] = {0x56, 0x67, 0x67, 0xf4, 0x67};
constexpr uint8_t kTestIk[] = {[0 ... 15] = 0x65};
constexpr uint8_t kTestCk[] = {[0 ... 15] = 0x45};
constexpr uint8_t kTestIdentity[] = {0x45, 0x67, 0x98, 0x67, 0x56};
constexpr uint32_t kTestKeyMgmt = (ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK |
                                   ISupplicantStaNetwork::KeyMgmtMask::WPA_EAP);
constexpr uint32_t kTestProto = (ISupplicantStaNetwork::ProtoMask::OSEN |
                                 ISupplicantStaNetwork::ProtoMask::RSN);
constexpr uint32_t kTestAuthAlg = (ISupplicantStaNetwork::AuthAlgMask::OPEN |
                                   ISupplicantStaNetwork::AuthAlgMask::SHARED);
constexpr uint32_t kTestGroupCipher =
    (ISupplicantStaNetwork::GroupCipherMask::CCMP |
     ISupplicantStaNetwork::GroupCipherMask::WEP104);
constexpr uint32_t kTestPairwiseCipher =
    (ISupplicantStaNetwork::PairwiseCipherMask::CCMP |
     ISupplicantStaNetwork::PairwiseCipherMask::TKIP);
}  // namespace

class SupplicantStaNetworkHidlTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        EXPECT_TRUE(turnOnExcessiveLogging());
        sta_network_ = createSupplicantStaNetwork();
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
    EXPECT_NE(nullptr, createSupplicantStaNetwork().get());
    stopSupplicant();
}

/*
 * SetGet
 * Tests out the various setter/getter methods.
 */
TEST_F(SupplicantStaNetworkHidlTest, SetGet) {
    std::vector<uint8_t> set_ssid(kTestSsidStr,
                                  kTestSsidStr + strlen(kTestSsidStr));
    sta_network_->setSsid(set_ssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->getSsid(
        [&](const SupplicantStatus& status, const hidl_vec<uint8_t>& get_ssid) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(set_ssid, std::vector<uint8_t>(get_ssid));
        });

    std::array<uint8_t, 6> set_bssid;
    memcpy(set_bssid.data(), kTestBssid, set_bssid.size());
    sta_network_->setBssid(set_bssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->getBssid([&](const SupplicantStatus& status,
                               const hidl_array<uint8_t, 6>& get_bssid_hidl) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        std::array<uint8_t, 6> get_bssid;
        memcpy(get_bssid.data(), get_bssid_hidl.data(), get_bssid.size());
        EXPECT_EQ(set_bssid, get_bssid);
    });

    sta_network_->setKeyMgmt(kTestKeyMgmt, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->getKeyMgmt(
        [&](const SupplicantStatus& status, uint32_t key_mgmt) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(key_mgmt, kTestKeyMgmt);
        });

    sta_network_->setProto(kTestProto, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->getProto([&](const SupplicantStatus& status, uint32_t proto) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(proto, kTestProto);
    });

    sta_network_->setAuthAlg(kTestAuthAlg, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->getAuthAlg(
        [&](const SupplicantStatus& status, uint32_t auth_alg) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(auth_alg, kTestAuthAlg);
        });

    sta_network_->setGroupCipher(
        kTestGroupCipher, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getGroupCipher(
        [&](const SupplicantStatus& status, uint32_t group_cipher) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(group_cipher, kTestGroupCipher);
        });

    sta_network_->setPairwiseCipher(
        kTestPairwiseCipher, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getPairwiseCipher(
        [&](const SupplicantStatus& status, uint32_t pairwise_cipher) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(pairwise_cipher, kTestPairwiseCipher);
        });

    sta_network_->setPskPassphrase(
        kTestPsk, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getPskPassphrase(
        [&](const SupplicantStatus& status, const hidl_string& psk) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(kTestPsk, std::string(psk.c_str()));
        });

    for (uint32_t i = 0;
         i < static_cast<uint32_t>(
                 ISupplicantStaNetwork::ParamSizeLimits::WEP_KEYS_MAX_NUM);
         i++) {
        std::vector<uint8_t> set_wep_key(std::begin(kTestWepKey),
                                         std::end(kTestWepKey));
        sta_network_->setWepKey(
            i, set_wep_key, [](const SupplicantStatus& status) {
                EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            });
        sta_network_->getWepKey(i, [&](const SupplicantStatus& status,
                                       const hidl_vec<uint8_t>& get_wep_key) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(set_wep_key, std::vector<uint8_t>(get_wep_key));
        });
    }

    ISupplicantStaNetwork::EapMethod set_eap_method =
        ISupplicantStaNetwork::EapMethod::PEAP;
    sta_network_->setEapMethod(
        set_eap_method, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getEapMethod(
        [&](const SupplicantStatus& status,
            ISupplicantStaNetwork::EapMethod eap_method) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(set_eap_method, eap_method);
        });

    ISupplicantStaNetwork::EapPhase2Method set_eap_phase2_method =
        ISupplicantStaNetwork::EapPhase2Method::PAP;
    sta_network_->setEapPhase2Method(
        set_eap_phase2_method, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getEapPhase2Method(
        [&](const SupplicantStatus& status,
            ISupplicantStaNetwork::EapPhase2Method eap_phase2_method) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            EXPECT_EQ(set_eap_phase2_method, eap_phase2_method);
        });

    std::vector<uint8_t> set_eap_passwd(
        kTestEapPasswdStr, kTestEapPasswdStr + strlen(kTestEapPasswdStr));
    sta_network_->setEapPassword(
        set_eap_passwd, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    sta_network_->getEapPassword([&](const SupplicantStatus& status,
                                     const hidl_vec<uint8_t>& get_eap_passwd) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        EXPECT_EQ(set_eap_passwd, std::vector<uint8_t>(get_eap_passwd));
    });
}

/*
 * Enable
 */
TEST_F(SupplicantStaNetworkHidlTest, Enable) {
    // wpa_supplicant doesn't perform any connection initiation
    // unless atleast the Ssid and Ket mgmt params are set.
    std::vector<uint8_t> set_ssid(kTestSsidStr,
                                  kTestSsidStr + strlen(kTestSsidStr));
    sta_network_->setSsid(set_ssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->setKeyMgmt(kTestKeyMgmt, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->enable(false, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->enable(true, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Disable
 */
TEST_F(SupplicantStaNetworkHidlTest, Disable) {
    // wpa_supplicant doesn't perform any connection initiation
    // unless atleast the Ssid and Ket mgmt params are set.
    std::vector<uint8_t> set_ssid(kTestSsidStr,
                                  kTestSsidStr + strlen(kTestSsidStr));
    sta_network_->setSsid(set_ssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->setKeyMgmt(kTestKeyMgmt, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->disable([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Select.
 */
TEST_F(SupplicantStaNetworkHidlTest, Select) {
    // wpa_supplicant doesn't perform any connection initiation
    // unless atleast the Ssid and Ket mgmt params are set.
    std::vector<uint8_t> set_ssid(kTestSsidStr,
                                  kTestSsidStr + strlen(kTestSsidStr));
    sta_network_->setSsid(set_ssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
    sta_network_->setKeyMgmt(kTestKeyMgmt, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    sta_network_->select([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * SendNetworkEapSimGsmAuthResponse
 */
TEST_F(SupplicantStaNetworkHidlTest, SendNetworkEapSimGsmAuthResponse) {
    ISupplicantStaNetwork::NetworkResponseEapSimGsmAuthParams params;
    memcpy(params.kc.data(), kTestKc, params.kc.size());
    memcpy(params.sres.data(), kTestSres, params.sres.size());
    sta_network_->sendNetworkEapSimGsmAuthResponse(
        params, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SendNetworkEapSimUmtsAuthResponse
 */
TEST_F(SupplicantStaNetworkHidlTest, SendNetworkEapSimUmtsAuthResponse) {
    ISupplicantStaNetwork::NetworkResponseEapSimUmtsAuthParams params;
    params.res = std::vector<uint8_t>(kTestRes, kTestRes + sizeof(kTestRes));
    memcpy(params.ik.data(), kTestIk, params.ik.size());
    memcpy(params.ck.data(), kTestCk, params.ck.size());
    sta_network_->sendNetworkEapSimUmtsAuthResponse(
        params, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SendNetworkEapIdentityResponse
 */
TEST_F(SupplicantStaNetworkHidlTest, SendNetworkEapIdentityResponse) {
    sta_network_->sendNetworkEapIdentityResponse(
        std::vector<uint8_t>(kTestIdentity,
                             kTestIdentity + sizeof(kTestIdentity)),
        [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}
