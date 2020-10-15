/*
 * Copyright (C) 2017 The Android Open Source Project
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

#define LOG_TAG "VtsOffloadControlV1_1TargetTest"

#include <OffloadControlTestV1_0.h>
#include <VtsHalHidlTargetCallbackBase.h>
#include <android/hardware/tetheroffload/config/1.0/IOffloadConfig.h>
#include <android/hardware/tetheroffload/control/1.0/IOffloadControl.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using android::hardware::tetheroffload::config::V1_0::IOffloadConfig;
using android::hardware::tetheroffload::control::V1_0::IOffloadControl;

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_0_NotInitedHal);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(OffloadControlTestV1_0_InitedHal);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_0_NotInitedHal,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);

INSTANTIATE_TEST_CASE_P(
        PerInstance, OffloadControlTestV1_0_InitedHal,
        testing::Combine(testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadConfig::descriptor)),
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IOffloadControl::descriptor))),
        android::hardware::PrintInstanceTupleNameToString<>);
