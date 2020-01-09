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
#include <android/hardware/power/IPower.h>
#include <android/hardware/power/Mode.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <future>

using android::ProcessState;
using android::sp;
using android::String16;
using android::binder::Status;
using android::hardware::power::Boost;
using android::hardware::power::IPower;
using android::hardware::power::Mode;

const std::vector<Boost> kBoosts{android::enum_range<Boost>().begin(),
                                 android::enum_range<Boost>().end()};

const std::vector<Mode> kModes{android::enum_range<Mode>().begin(),
                               android::enum_range<Mode>().end()};

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
}

TEST_P(PowerAidl, supportMode) {
    for (const auto& mode : kModes) {
        bool supported;
        ASSERT_TRUE(power->supportMode(mode, &supported).isOk());
    }
}

TEST_P(PowerAidl, setBoost) {
    for (const auto& boost : kBoosts) {
        ASSERT_TRUE(power->setBoost(boost, 0).isOk());
        ASSERT_TRUE(power->setBoost(boost, 1000).isOk());
        ASSERT_TRUE(power->setBoost(boost, -1).isOk());
    }
}

TEST_P(PowerAidl, supportBoost) {
    for (const auto& boost : kBoosts) {
        bool supported;
        ASSERT_TRUE(power->supportBoost(boost, &supported).isOk());
    }
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
