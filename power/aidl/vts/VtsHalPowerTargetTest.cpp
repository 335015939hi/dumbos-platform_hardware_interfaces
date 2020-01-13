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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android/hardware/power/Boost.h>
#include <android/hardware/power/Hint.h>
#include <android/hardware/power/IPower.h>
#include <android/hardware/power/Mode.h>
#include <android/hardware/power/ThreadHint.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <future>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::power::Boost;
using android::hardware::power::Hint;
using android::hardware::power::IPower;
using android::hardware::power::Mode;
using android::hardware::power::ThreadHint;

const std::vector<Boost> kBoosts{android::enum_range<Boost>().begin(),
                                 android::enum_range<Boost>().end()};

const std::vector<Mode> kModes{android::enum_range<Mode>().begin(),
                               android::enum_range<Mode>().end()};

const std::vector<Hint> kHints{android::enum_range<Hint>().begin(),
                               android::enum_range<Hint>().end()};

const std::vector<ThreadHint> kThreadHints{android::enum_range<ThreadHint>().begin(),
                                           android::enum_range<ThreadHint>().end()};

const std::vector<Boost> kInvalidBoosts = {
        static_cast<Boost>(static_cast<int32_t>(kBoosts.front()) - 1),
        static_cast<Boost>(static_cast<int32_t>(kBoosts.back()) + 1),
};

const std::vector<Mode> kInvalidModes = {
        static_cast<Mode>(static_cast<int32_t>(kModes.front()) - 1),
        static_cast<Mode>(static_cast<int32_t>(kModes.back()) + 1),
};

const std::vector<Hint> kInvalidHints = {
        static_cast<Hint>(static_cast<int32_t>(kHints.front()) - 1),
        static_cast<Hint>(static_cast<int32_t>(kHints.back()) + 1),
};

class PowerAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        power = android::waitForDeclaredService<IPower>(String16(GetParam().c_str()));
        ASSERT_NE(power, nullptr);
    }

    sp<IPower> power;
};

TEST_P(PowerAidl, setMode) {
    for (const auto& mode : kModes) {
        ASSERT_TRUE(power->setMode(mode, true).isOk());
        ASSERT_TRUE(power->setMode(mode, false).isOk());
    }
    for (const auto& mode : kInvalidModes) {
        ASSERT_TRUE(power->setMode(mode, true).isOk());
        ASSERT_TRUE(power->setMode(mode, false).isOk());
    }
}

TEST_P(PowerAidl, isModeSupported) {
    for (const auto& mode : kModes) {
        bool supported;
        ASSERT_TRUE(power->isModeSupported(mode, &supported).isOk());
    }
    for (const auto& mode : kInvalidModes) {
        bool supported;
        ASSERT_TRUE(power->isModeSupported(mode, &supported).isOk());
        // Should return false for values outside enum
        ASSERT_FALSE(supported);
    }
}

TEST_P(PowerAidl, setBoost) {
    for (const auto& boost : kBoosts) {
        ASSERT_TRUE(power->setBoost(boost, 0).isOk());
        ASSERT_TRUE(power->setBoost(boost, 1000).isOk());
        ASSERT_TRUE(power->setBoost(boost, -1).isOk());
    }
    for (const auto& boost : kInvalidBoosts) {
        ASSERT_TRUE(power->setBoost(boost, 0).isOk());
        ASSERT_TRUE(power->setBoost(boost, 1000).isOk());
        ASSERT_TRUE(power->setBoost(boost, -1).isOk());
    }
}

TEST_P(PowerAidl, isBoostSupported) {
    for (const auto& boost : kBoosts) {
        bool supported;
        ASSERT_TRUE(power->isBoostSupported(boost, &supported).isOk());
    }
    for (const auto& boost : kInvalidBoosts) {
        bool supported;
        ASSERT_TRUE(power->isBoostSupported(boost, &supported).isOk());
        // Should return false for values outside enum
        ASSERT_FALSE(supported);
    }
}

TEST_P(PowerAidl, fixedPerformanceSupportsAllOrNone) {
    bool supportsMaxSustainable;
    const auto result = power->isModeSupported(Mode::FIXED_PERFORMANCE_MAXIMUM_SUSTAINABLE,
                                               &supportsMaxSustainable);
    ASSERT_TRUE(result.isOk());

    // If one FIXED_PERFORMANCE mode is supported, both should be.
    bool supported;
    ASSERT_TRUE(power->isModeSupported(Mode::FIXED_PERFORMANCE_MINIMUM, &supported).isOk());
    ASSERT_EQ(supported, supportsMaxSustainable);
}

TEST_P(PowerAidl, isHintSupported) {
    for (const auto& hint : kHints) {
        bool supported;
        ASSERT_TRUE(power->isHintSupported(hint, &supported).isOk());
    }
    for (const auto& hint : kInvalidHints) {
        bool supported;
        ASSERT_TRUE(power->isHintSupported(hint, &supported).isOk());
        // Should return false for values outside enum
        ASSERT_FALSE(supported);
    }
}

TEST_P(PowerAidl, setWorkloadPeriod) {
    const pid_t tid = gettid();
    ASSERT_TRUE(power->setWorkloadPeriod(tid, 10000).isOk());
    ASSERT_TRUE(power->setWorkloadPeriod(tid, 33333).isOk());
}

TEST_P(PowerAidl, notifyLoadChanged) {
    const pid_t tid = gettid();
    ASSERT_TRUE(power->notifyLoadChanged(tid, 0.5f, 0.5f).isOk());
    ASSERT_TRUE(power->notifyLoadChanged(tid, 0.5f, 2.0f).isOk());
    ASSERT_TRUE(power->notifyLoadChanged(tid, 2.0f, 0.5f).isOk());
    ASSERT_TRUE(power->notifyLoadChanged(tid, 2.0f, 2.0f).isOk());
}

TEST_P(PowerAidl, setThreadHint) {
    const pid_t tid = gettid();
    ASSERT_TRUE(power->setThreadHint(tid, ThreadHint::NONE).isOk());
    ASSERT_TRUE(power->setThreadHint(tid, ThreadHint::HIGH_UTILIZATION).isOk());
    ASSERT_TRUE(power->setThreadHint(tid, ThreadHint::LOW_LATENCY).isOk());
    const int32_t bothHints = static_cast<int32_t>(ThreadHint::HIGH_UTILIZATION) |
                              static_cast<int32_t>(ThreadHint::LOW_LATENCY);
    ASSERT_TRUE(power->setThreadHint(tid, static_cast<ThreadHint>(bothHints)).isOk());
    ASSERT_TRUE(power->setThreadHint(tid, ThreadHint::NONE).isOk());
}

INSTANTIATE_TEST_SUITE_P(Power, PowerAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IPower::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
