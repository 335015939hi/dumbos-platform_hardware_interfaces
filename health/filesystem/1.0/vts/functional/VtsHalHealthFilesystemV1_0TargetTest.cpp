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

#define LOG_TAG "health_filesystem_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/health/filesystem/1.0/IFileSystem.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::Return;
using ::android::hardware::health::filesystem::V1_0::IFileSystem;
using ::android::hardware::health::filesystem::V1_0::Result;
using ::android::hardware::health::filesystem::V1_0::toString;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.description();

// Test environment for Health Filesystem HIDL HAL.
class HealthFilesystemHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static HealthFilesystemHidlEnvironment* Instance() {
        static HealthFilesystemHidlEnvironment* instance = new HealthFilesystemHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IFileSystem>(); }

   private:
    HealthFilesystemHidlEnvironment() {}
};

class HealthFilesystemHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        fs = ::testing::VtsHalHidlTargetTestBase::getService<IFileSystem>(
            HealthFilesystemHidlEnvironment::Instance()->getServiceName<IFileSystem>());

        ASSERT_NE(fs, nullptr);
        LOG(INFO) << "Service is remote " << fs->isRemote();
    }

    sp<IFileSystem> fs;
};

/**
 * Ensure manual garbage collection works.
 */
TEST_F(HealthFilesystemHidlTest, TestSupported) {
    auto ret = fs->manualGarbageCollect();
    ASSERT_OK(ret);
    Result result = ret;
    EXPECT_TRUE(result == Result::SUCCESS || result == Result::NOT_SUPPORTED)
        << "manualGarbageCollect returns " << toString(result);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(HealthFilesystemHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    HealthFilesystemHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
