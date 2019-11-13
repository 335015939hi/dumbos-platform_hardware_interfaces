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

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android-base/file.h>
#include <android-base/strings.h>
#include <android/hardware/boot/power/IPowerManagement.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

using android::ProcessState;
using android::String16;
using android::hardware::boot::power::IPowerManagement;

class PowerManagementAidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        power_ = android::waitForService<IPowerManagement>(String16(GetParam().c_str()));
        ASSERT_TRUE(power_ != nullptr) << "Failed to get PowerManagement service.";
    }

    // Clean up the /misc after the test.
    void TearDown() override {
        bool status = false;
        ASSERT_TRUE(power_->clearWarmResetFlag(&status).isOk());
        ASSERT_TRUE(status);
    }

  protected:
    android::sp<IPowerManagement> power_;
};

TEST_P(PowerManagementAidlTest, setClearWarmResetFlag) {
    constexpr char warm_reset_path[] = "/sys/module/msm_poweroff/parameters/warm_reset";

    // Write the flag.
    bool status = false;
    ASSERT_TRUE(power_->setWarmResetFlag(&status).isOk());
    ASSERT_TRUE(status);
    std::string content;
    ASSERT_TRUE(android::base::ReadFileToString(warm_reset_path, &content));
    ASSERT_TRUE(android::base::StartsWithIgnoreCase(content, "Y"));

    ASSERT_TRUE(power_->clearWarmResetFlag(&status).isOk());
    ASSERT_TRUE(status);
    ASSERT_TRUE(android::base::ReadFileToString(warm_reset_path, &content));
    ASSERT_TRUE(android::base::StartsWithIgnoreCase(content, "N"));
}

INSTANTIATE_TEST_SUITE_P(
        PerInstance, PowerManagementAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(IPowerManagement::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
