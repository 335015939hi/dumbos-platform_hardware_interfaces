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

class OffloadControlTestV1_1_HalNotStarted : public OffloadControlTestV1_0_HalNotStarted {
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
TEST_P(OffloadControlTestV1_1_HalNotStarted, SetDataWarningAndLimitWithoutInitReturnsFalse) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(upstream, 5000ULL, 5000ULL,
                                                                      ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

class OffloadControlTestV1_1_HalStarted : public OffloadControlTestV1_0_HalStarted {
  public:
    sp<android::hardware::tetheroffload::control::V1_1::IOffloadControl> getControlV1_1() {
        // The cast is safe since only devices with V1.1+ HAL will be enumerated and pass in to the
        // test.
        return static_cast<android::hardware::tetheroffload::control::V1_1::IOffloadControl*>(
                control.get());
    }
};

/*
 * Tests for IOffloadControl::setDataWarningAndLimit().
 */

// Test that setDataWarningAndLimit() for an empty interface name fails.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitEmptyUpstreamIfaceFails) {
    const hidl_string upstream("");
    const Return<void> ret = getControlV1_1()->setDataWarningAndLimit(upstream, 12345ULL, 67890ULL,
                                                                      ASSERT_FALSE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitNonZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret1 = getControlV1_1()->setDataWarningAndLimit(upstream, 4000ULL, 5000ULL,
                                                                       ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret1.isOk());
    // Verify warning greater than limit is also accepted by hardware.
    const Return<void> ret2 = getControlV1_1()->setDataWarningAndLimit(upstream, 5000ULL, 4000ULL,
                                                                       ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret2.isOk());
}

// TEST_IFACE is presumed to exist on the device and be up. No packets
// are ever actually caused to be forwarded.
TEST_P(OffloadControlTestV1_1_HalStarted, SetDataWarningAndLimitZeroOk) {
    const hidl_string upstream(TEST_IFACE);
    const Return<void> ret =
            getControlV1_1()->setDataWarningAndLimit(upstream, 0ULL, 0ULL, ASSERT_TRUE_CALLBACK);
    EXPECT_TRUE(ret.isOk());
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_1_HalNotStarted);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_1_HalStarted);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_1_HalNotStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::tetheroffload::control::V1_1::IOffloadControl::
                                         descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_1_HalStarted,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 android::hardware::tetheroffload::control::V1_1::IOffloadControl::
                                         descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
