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

#include <android/hardware/wifi/supplicant/1.0/ISupplicantP2pIface.h>

#include "supplicant_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_array;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantNetworkId;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;

namespace {
constexpr uint8_t kTestSsidPostfix[] = {'t', 'e', 's', 't'};
constexpr uint8_t kTestMacAddr[] = {0x56, 0x67, 0x67, 0xf4, 0x56, 0x92};
constexpr uint8_t kTestPeerMacAddr[] = {0x56, 0x67, 0x55, 0xf4, 0x56, 0x92};
constexpr char kTestConnectPin[] = "0x34556665";
constexpr char kTestGroupIfName[] = "TestGroup";
constexpr uint32_t kTestConnectGoIntent = 6;
constexpr uint32_t kTestFindTimeout = 5;
constexpr SupplicantNetworkId kTestNetworkId = 5;
constexpr uint32_t kTestChannel = 1;
constexpr uint32_t kTestOperatingClass = 81;
constexpr uint32_t kTestFreqRange[] = {2412, 2432};
constexpr uint32_t kTestExtListenPeriod = 400;
constexpr uint32_t kTestExtListenInterval = 400;
}  // namespace

class SupplicantP2pIfaceHidlTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        EXPECT_TRUE(turnOnExcessiveLogging());
        p2p_iface_ = getSupplicantP2pIface();
        EXPECT_NE(p2p_iface_.get(), nullptr);
    }

    virtual void TearDown() override { stopSupplicant(); }

   protected:
    sp<ISupplicantP2pIface> p2p_iface_;
};

/*
 * Create:
 * Ensures that an instance of the ISupplicantP2pIface proxy object is
 * successfully created.
 */
TEST(SupplicantP2pIfaceHidlTestNoFixture, Create) {
    startSupplicantAndWaitForHidlService();
    EXPECT_NE(nullptr, getSupplicantP2pIface().get());
    stopSupplicant();
}

/*
 * GetDeviceAddress
 */
TEST_F(SupplicantP2pIfaceHidlTest, GetDeviceAddress) {
    p2p_iface_->getDeviceAddress(
        [](const SupplicantStatus& status,
           const hidl_array<uint8_t, 6>& /* mac_addr */) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetSsidPostfix
 */
TEST_F(SupplicantP2pIfaceHidlTest, SetSsidPostfix) {
    std::vector<uint8_t> ssid(kTestSsidPostfix,
                              kTestSsidPostfix + sizeof(kTestSsidPostfix));
    p2p_iface_->setSsidPostfix(ssid, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Find
 */
TEST_F(SupplicantP2pIfaceHidlTest, Find) {
    p2p_iface_->find(kTestFindTimeout, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * StopFind
 */
TEST_F(SupplicantP2pIfaceHidlTest, StopFind) {
    p2p_iface_->find(kTestFindTimeout, [](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });

    p2p_iface_->stopFind([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Flush
 */
TEST_F(SupplicantP2pIfaceHidlTest, Flush) {
    p2p_iface_->flush([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * Connect
 */
TEST_F(SupplicantP2pIfaceHidlTest, Connect) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->connect(
        mac_addr, ISupplicantP2pIface::WpsProvisionMethod::PBC, kTestConnectPin,
        false, false, kTestConnectGoIntent,
        [](const SupplicantStatus& status, const hidl_string& /* pin */) {
            // This is not going to work with fake values.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * CancelConnect
 */
TEST_F(SupplicantP2pIfaceHidlTest, CancelConnect) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->connect(
        mac_addr, ISupplicantP2pIface::WpsProvisionMethod::PBC, kTestConnectPin,
        false, false, kTestConnectGoIntent,
        [](const SupplicantStatus& status, const hidl_string& /* pin */) {
            // This is not going to work with fake values.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });

    p2p_iface_->cancelConnect([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
    });
}

/*
 * ProvisionDiscovery
 */
TEST_F(SupplicantP2pIfaceHidlTest, ProvisionDiscovery) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->provisionDiscovery(
        mac_addr, ISupplicantP2pIface::WpsProvisionMethod::PBC,
        [](const SupplicantStatus& status) {
            // This is not going to work with fake values.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * AddGroup
 */
TEST_F(SupplicantP2pIfaceHidlTest, AddGroup) {
    p2p_iface_->addGroup(false, kTestNetworkId,
                         [](const SupplicantStatus& /* status */) {
                             // TODO: Figure out the initialization sequence for
                             // this to work.
                             // EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                             // status.code);
                         });
}

/*
 * Reject
 */
TEST_F(SupplicantP2pIfaceHidlTest, Reject) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->reject(mac_addr, [](const SupplicantStatus& status) {
        // This is not going to work with fake values.
        EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
    });
}

/*
 * Invite
 */
TEST_F(SupplicantP2pIfaceHidlTest, Invite) {
    std::array<uint8_t, 6> go_addr;
    memcpy(go_addr.data(), kTestMacAddr, go_addr.size());
    std::array<uint8_t, 6> peer_addr;
    memcpy(peer_addr.data(), kTestPeerMacAddr, peer_addr.size());
    p2p_iface_->invite(kTestGroupIfName, go_addr, peer_addr,
                       [](const SupplicantStatus& status) {
                           // This is not going to work with fake values.
                           EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN,
                                     status.code);
                       });
}

/*
 * Reinvoke
 */
TEST_F(SupplicantP2pIfaceHidlTest, Reinvoke) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->reinvoke(
        kTestNetworkId, mac_addr, [](const SupplicantStatus& status) {
            // This is not going to work with fake values.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN,
                      status.code);
        });
}

/*
 * ConfigureExtListen
 */
TEST_F(SupplicantP2pIfaceHidlTest, ConfigureExtListen) {
    p2p_iface_->configureExtListen(kTestExtListenPeriod, kTestExtListenInterval,
                                   [](const SupplicantStatus& status) {
                                       EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                                 status.code);
                                   });
}

/*
 * SetListenChannel
 */
TEST_F(SupplicantP2pIfaceHidlTest, SetListenChannel) {
    p2p_iface_->setListenChannel(
        kTestChannel, kTestOperatingClass, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * SetDisallowedFrequencies
 */
TEST_F(SupplicantP2pIfaceHidlTest, SetDisallowedFrequencies) {
    std::vector<ISupplicantP2pIface::FreqRange> ranges = {
        {kTestFreqRange[0], kTestFreqRange[1]}};
    p2p_iface_->setDisallowedFrequencies(
        ranges, [](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}

/*
 * GetSsid
 */
TEST_F(SupplicantP2pIfaceHidlTest, GetSsid) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->getSsid(mac_addr, [](const SupplicantStatus& status,
                                     const hidl_vec<uint8_t>& /* ssid */) {
        // This is not going to work with fake values.
        EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
    });
}

/*
 * GetGroupCapability
 */
TEST_F(SupplicantP2pIfaceHidlTest, GetGroupCapability) {
    std::array<uint8_t, 6> mac_addr;
    memcpy(mac_addr.data(), kTestMacAddr, mac_addr.size());
    p2p_iface_->getGroupCapability(
        mac_addr, [](const SupplicantStatus& status, uint32_t /* caps */) {
            // This is not going to work with fake values.
            EXPECT_EQ(SupplicantStatusCode::FAILURE_UNKNOWN, status.code);
        });
}

/*
 * FlushServices
 */
TEST_F(SupplicantP2pIfaceHidlTest, FlushServices) {
    p2p_iface_->flushServices([](const SupplicantStatus& status) {
        EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
    });
}

/*
 * SetMiracastMode
 */
TEST_F(SupplicantP2pIfaceHidlTest, SetMiracastMode) {
    p2p_iface_->setMiracastMode(ISupplicantP2pIface::MiracastMode::DISABLED,
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
    p2p_iface_->setMiracastMode(ISupplicantP2pIface::MiracastMode::SOURCE,
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
    p2p_iface_->setMiracastMode(ISupplicantP2pIface::MiracastMode::SINK,
                                [](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });
}
