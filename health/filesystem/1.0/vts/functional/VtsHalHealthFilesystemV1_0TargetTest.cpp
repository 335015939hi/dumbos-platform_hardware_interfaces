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
#include <hidl/HidlTransportSupport.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::health::filesystem::V1_0::IFileSystem;
using ::android::hardware::health::filesystem::V1_0::IGarbageCollectCallback;
using ::android::hardware::health::filesystem::V1_0::Result;
using ::android::hardware::health::filesystem::V1_0::toString;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.description();

// Dev GC timeout. This is the timeout used by vold.
const uint64_t DEVGC_TIMEOUT_SEC = 120;

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
 * Ensure garbage collection works on null callback.
 */
TEST_F(HealthFilesystemHidlTest, GcNullCallback) {
    auto ret = fs->garbageCollect(DEVGC_TIMEOUT_SEC, nullptr);
    ASSERT_OK(ret);
}

class GcCallback : public IGarbageCollectCallback {
   public:
    Return<void> onFinish(Result result) override {
        std::unique_lock<std::mutex> lock(mMutex);
        mFinished = true;
        mResult = result;
        lock.unlock();
        mCv.notify_all();
        return Void();
    }
    void wait(uint64_t seconds) {
        std::unique_lock<std::mutex> lock(mMutex);
        mCv.wait_for(lock, std::chrono::seconds(seconds), [this] { return mFinished; });
    }

    bool isFinished() const { return mFinished; }
    Result getResult() const { return mResult; }

   private:
    std::mutex mMutex;
    std::condition_variable mCv;
    bool mFinished{false};
    Result mResult{Result::UNKNOWN_ERROR};
};

/**
 * Ensure garbage collection works on non-null callback.
 */
TEST_F(HealthFilesystemHidlTest, GcNonNullCallback) {
    sp<GcCallback> cb = new GcCallback();
    auto ret = fs->garbageCollect(DEVGC_TIMEOUT_SEC, cb);
    ASSERT_OK(ret);
    cb->wait(DEVGC_TIMEOUT_SEC);

    ASSERT_TRUE(cb->isFinished()) << "garbageCollect timeout after " << DEVGC_TIMEOUT_SEC
                                  << " seconds";
    ASSERT_EQ(Result::SUCCESS, cb->getResult());
}

int main(int argc, char** argv) {
    configureRpcThreadpool(1, false /* callerWillJoin*/);
    ::testing::AddGlobalTestEnvironment(HealthFilesystemHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    HealthFilesystemHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
