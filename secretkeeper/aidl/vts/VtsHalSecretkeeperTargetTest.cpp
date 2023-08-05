/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <aidl/android/hardware/secretkeeper/ISecretkeeper.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using ::aidl::android::hardware::secretkeeper::ISecretkeeper;

using ::ndk::SpAIBinder;

class SkAidlTest : public ::testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        SpAIBinder binder = SpAIBinder(AServiceManager_waitForService(GetParam().c_str()));
        sk = ISecretkeeper::fromBinder(binder);
        ASSERT_NE(sk, nullptr);
    }

    void TearDown() override {}

    std::shared_ptr<ISecretkeeper> sk;
};

TEST_P(SkAidlTest, store_read_works) {
    std::vector<std::uint8_t> in1 = {'a', 'b'};
    std::vector<std::uint8_t> in2 = {'c', 'd'};
    std::vector<std::uint8_t> out;

    const std::vector<std::uint8_t> auth_det = {'x', 'y'};

    sk->store(in1);
    sk->read(auth_det, &out);
    ASSERT_EQ(in1, out);

    sk->store(in2);
    sk->read(auth_det, &out);
    ASSERT_EQ(in2, out);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(SkAidlTest);
INSTANTIATE_TEST_SUITE_P(
        SecretkeeperPrefix, SkAidlTest,
        testing::ValuesIn(android::getAidlHalInstanceNames(ISecretkeeper::descriptor)),
        android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
