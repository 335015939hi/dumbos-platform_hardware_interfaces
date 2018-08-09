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

#define LOG_TAG "health_storage_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/health/storage/1.0/IStorage.h>
#include <hidl/HidlTransportSupport.h>
#include <unistd.h>

using ::android::sp;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::health::storage::V1_0::IGarbageCollectCallback;
using ::android::hardware::health::storage::V1_0::IStorage;
using ::android::hardware::health::storage::V1_0::Result;
using ::android::hardware::health::storage::V1_0::toString;

#define ASSERT_OK(ret) ASSERT_TRUE(ret.isOk()) << ret.description();

// Dev GC timeout. This is the timeout used by vold.
const uint64_t DEVGC_TIMEOUT_SEC = 120;

// Test environment for Health Storage HIDL HAL.
class HealthStorageHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static HealthStorageHidlEnvironment* Instance() {
        static HealthStorageHidlEnvironment* instance = new HealthStorageHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IStorage>(); }

   private:
    HealthStorageHidlEnvironment() {}
};

class HealthStorageHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        fs = ::testing::VtsHalHidlTargetTestBase::getService<IStorage>(
            HealthStorageHidlEnvironment::Instance()->getServiceName<IStorage>());

        ASSERT_NE(fs, nullptr);
        LOG(INFO) << "Service is remote " << fs->isRemote();
    }

    sp<IStorage> fs;
};

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
 * Ensure garbage collection works on null callback.
 */
TEST_F(HealthStorageHidlTest, GcNullCallback) {
    auto ret = fs->garbageCollect(DEVGC_TIMEOUT_SEC, nullptr);
    ASSERT_OK(ret);
}

/**
 * Ensure garbage collection works on non-null callback.
 */
TEST_F(HealthStorageHidlTest, GcNonNullCallback) {
    sp<GcCallback> cb = new GcCallback();
    auto ret = fs->garbageCollect(DEVGC_TIMEOUT_SEC, cb);
    ASSERT_OK(ret);
    cb->wait(DEVGC_TIMEOUT_SEC);

    if (cb->isFinished()) {
        ASSERT_EQ(Result::SUCCESS, cb->getResult());
    } else {
        LOG(INFO) << "garbageCollect timeout after " << DEVGC_TIMEOUT_SEC << " seconds";
    }
}

int main(int argc, char** argv) {
    configureRpcThreadpool(1, false /* callerWillJoin*/);
    ::testing::AddGlobalTestEnvironment(HealthStorageHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    HealthStorageHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
