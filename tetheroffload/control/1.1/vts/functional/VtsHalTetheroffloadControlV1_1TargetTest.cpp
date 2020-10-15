/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <OffloadControlTestV1_0.h>
#include <android/hardware/tetheroffload/control/1.1/IOffloadControl.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

constexpr const char* TEST_IFACE = "rmnet_data0";

class OffloadControlTestV1_1_NotInitedHal : public OffloadControlTestV1_0_NotInitedHal {
  public:
    sp<android::hardware::tetheroffload::control::V1_1::IOffloadControl> getControlV1_1() {
        // The cast is safe since only devices with V1.1+ HAL will be enumerated and pass in to the
        // test.
        return static_cast<android::hardware::tetheroffload::control::V1_1::IOffloadControl*>(
                control.get());
    }
};

// Check that calling setDataWarningAndLimit() without first having called initOffload() returns
// false.
TEST_P(OffloadControlTestV1_1_NotInitedHal, SetDataWarningAndLimitWithoutInitReturnsFalse) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(upstream, 5000ULL, 5000ULL,
                                                                      ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_1_NotInitedHal);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_0_InitedHal);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_1_NotInitedHal,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::tetheroffload::control::V1_1::IOffloadControl::
                                         descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_0_InitedHal,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::tetheroffload::control::V1_1::IOffloadControl::
                                         descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
