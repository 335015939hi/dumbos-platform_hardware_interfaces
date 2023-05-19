/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/rpmb/IRpmb.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::aidl::android::hardware::rpmb::IRpmb;
using ::aidl::android::hardware::rpmb::RpmbGeometry;
using ::aidl::android::hardware::rpmb::RpmbMessageFrameNormal;

using ::ndk::SpAIBinder;

class RpmbAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder = SpAIBinder(AServiceManager_waitForService(GetParam().c_str()));
        rpmb = IRpmb::fromBinder(binder);
        ASSERT_NE(rpmb, nullptr);
    }

    void TearDown() override {}

    std::shared_ptr<IRpmb> rpmb;
};
// TODO: test something meaningful.
TEST_P(RpmbAidlTest, ProgramKey) {
    ASSERT_NE(0, 1);
    std::vector<RpmbMessageFrameNormal> input(4);
    std::vector<RpmbMessageFrameNormal> output(3);
    RpmbGeometry geometry;
    rpmb->initialize_device(&geometry);
    rpmb->operate(input, &output);
    ASSERT_NE(1, 0);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(RpmbAidlTest);
INSTANTIATE_TEST_SUITE_P(RpmbPrefix, RpmbAidlTest,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IRpmb::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
