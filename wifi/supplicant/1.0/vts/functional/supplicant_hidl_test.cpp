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

#include <android/hardware/wifi/supplicant/1.0/ISupplicant.h>

#include "supplicant_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;

class SupplicantHidlTest : public ::testing::Test {
   public:
    virtual void SetUp() override {
        startSupplicantAndWaitForHidlService();
        supplicant_ = getSupplicant();
        EXPECT_NE(supplicant_.get(), nullptr);
    }

    virtual void TearDown() override { stopSupplicant(); }

   protected:
    sp<ISupplicant> supplicant_;
};

/*
 * Create:
 * Ensures that an instance of the ISupplicant proxy object is
 * successfully created.
 */
TEST(SupplicantHidlTestNoFixture, Create) {
    startSupplicantAndWaitForHidlService();
    EXPECT_NE(nullptr, getSupplicant().get());
    stopSupplicant();
}

/*
 * ListInterfaces
 */
TEST_F(SupplicantHidlTest, ListInterfaces) {
    std::vector<ISupplicant::IfaceInfo> ifaces;
    supplicant_->listInterfaces(
        [&](const SupplicantStatus& status,
            const hidl_vec<ISupplicant::IfaceInfo>& hidl_ifaces) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
            ifaces = hidl_ifaces;
        });
    // Check for atleast one STA and one P2P iface.
    bool is_sta_found = false;
    bool is_p2p_found = false;
    for (const auto& iface : ifaces) {
        if (iface.type == IfaceType::STA) {
            is_sta_found = true;
        } else if (iface.type == IfaceType::P2P) {
            is_p2p_found = true;
        }
    }
    EXPECT_TRUE(is_sta_found);
    EXPECT_TRUE(is_p2p_found);
}

/*
 * DebugParams
 */
TEST_F(SupplicantHidlTest, DebugParams) {
    bool show_timestamp = true;
    bool show_keys = true;
    ISupplicant::DebugLevel level = ISupplicant::DebugLevel::EXCESSIVE;

    supplicant_->setDebugParams(level,
                                true,  // show timestamps
                                true,  // show keys
                                [&](const SupplicantStatus& status) {
                                    EXPECT_EQ(SupplicantStatusCode::SUCCESS,
                                              status.code);
                                });

    EXPECT_EQ(level, supplicant_->getDebugLevel());
    EXPECT_EQ(show_timestamp, supplicant_->isDebugShowTimestampEnabled());
    EXPECT_EQ(show_keys, supplicant_->isDebugShowKeysEnabled());
}

/*
 * SetConcurrenyPriority
 */
TEST_F(SupplicantHidlTest, SetConcurrencyPriority) {
    supplicant_->setConcurrencyPriority(
        IfaceType::STA, [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
    supplicant_->setConcurrencyPriority(
        IfaceType::P2P, [&](const SupplicantStatus& status) {
            EXPECT_EQ(SupplicantStatusCode::SUCCESS, status.code);
        });
}
