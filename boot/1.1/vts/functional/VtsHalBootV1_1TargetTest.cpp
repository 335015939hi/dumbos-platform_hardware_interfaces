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

#define LOG_TAG "boot_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/boot/1.1/IBootControl.h>
#include <android/hardware/boot/1.1/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

#include <unistd.h>

using ::android::sp;
using ::android::hardware::hidl_enum_range;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::boot::V1_1::IBootControl;
using ::android::hardware::boot::V1_1::MergeStatus;

#define EXPECT_OK(ret) EXPECT_TRUE(ret.isOk())

class BootHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        boot = IBootControl::getService(GetParam());

        ASSERT_NE(boot, nullptr);
        LOG(INFO) << "Test is remote " << boot->isRemote();
    }

    sp<IBootControl> boot;
};

/**
 * Ensure merge status can be retrieved.
 */
TEST_P(BootHidlTest, GetSnapshotMergeStatus) {
    EXPECT_OK(boot->getSnapshotMergeStatus([](auto success, auto) { EXPECT_TRUE(success); }));
}

/**
 * Ensure merge status can be set to arbitrary value.
 */
TEST_P(BootHidlTest, SetSnapshotMergeStatus) {
    for (const auto value : hidl_enum_range<MergeStatus>()) {
        EXPECT_TRUE(boot->setSnapshotMergeStatus(value).withDefault(false));
        EXPECT_OK(boot->getSnapshotMergeStatus([&](auto success, auto new_value) {
            EXPECT_TRUE(success);
            EXPECT_EQ(value, new_value);
        }));
    }
}

INSTANTIATE_TEST_SUITE_P(
        , BootHidlTest,
        testing::ValuesIn(android::hardware::getAllHalInstanceNames(IBootControl::descriptor)),
        android::hardware::PrintInstanceNameToString);
