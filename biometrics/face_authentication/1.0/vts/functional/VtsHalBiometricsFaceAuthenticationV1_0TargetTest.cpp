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

#define LOG_TAG "face_authentication_hidl_hal_test"

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>
#include <android-base/logging.h>
#include <android/hardware/biometrics/face_authentication/1.0/IBiometricsFace.h>
#include <android/hardware/biometrics/face_authentication/1.0/IBiometricsFaceClientCallback.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

#include <cinttypes>
#include <future>
#include <utility>

using android::Condition;
using android::Mutex;
using android::sp;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::hardware::biometrics::face_authentication::V1_0::FaceAcquiredInfo;
using android::hardware::biometrics::face_authentication::V1_0::FaceError;
using android::hardware::biometrics::face_authentication::V1_0::IBiometricsFace;
using android::hardware::biometrics::face_authentication::V1_0::IBiometricsFaceClientCallback;
using android::hardware::biometrics::face_authentication::V1_0::RequestStatus;

namespace {

static const uint32_t kTimeout = 3;
static const std::chrono::seconds kTimeoutInSeconds = std::chrono::seconds(kTimeout);
static const uint32_t kUserId = 99;
// Assuming the following folder will be available for face authentication in Android P.
static const std::string kTmpDir = "/data/vendor/users/0/faceAuthdata/";
static const uint32_t kIterations = 1000;

// Wait for a callback to occur (signaled by the given future) up to the
// provided timeout. If the future is invalid or the callback does not come
// within the given time, returns false.
template <class ReturnType>
bool waitForCallback(std::future<ReturnType> future,
                     std::chrono::milliseconds timeout = kTimeoutInSeconds) {
    auto expiration = std::chrono::system_clock::now() + timeout;

    EXPECT_TRUE(future.valid());
    if (future.valid()) {
        std::future_status status = future.wait_until(expiration);
        EXPECT_NE(std::future_status::timeout, status) << "Timed out waiting for callback";
        if (status == std::future_status::ready) {
            return true;
        }
    }

    return false;
}

// Base callback implementation that just logs all callbacks by default
class FaceCallbackBase : public IBiometricsFaceClientCallback {
   public:
    // implement methods of IBiometricsFaceClientCallback
    virtual Return<void> onEnrollResult(uint64_t, uint32_t) override {
        ALOGD("Enroll callback called.");
        return Return<void>();
    }

    virtual Return<void> onAcquired(uint64_t, FaceAcquiredInfo, int32_t) override {
        ALOGD("Acquired callback called.");
        return Return<void>();
    }

    virtual Return<void> onAuthenticated(uint64_t, bool, const hidl_vec<uint8_t>&) override {
        ALOGD("Authenticated callback called.");
        return Return<void>();
    }

    virtual Return<void> onError(uint64_t, FaceError, int32_t) override {
        ALOGD("Error callback called.");
        EXPECT_TRUE(false);  // fail any test that triggers an error
        return Return<void>();
    }

    virtual Return<void> onRemoved(uint64_t) override {
        ALOGD("Removed callback called.");
        return Return<void>();
    }
};

class ErrorCallback : public FaceCallbackBase {
   public:
    ErrorCallback(bool filterErrors = false, FaceError errorType = FaceError::ERROR_NO_ERROR) {
        this->filterErrors = filterErrors;
        this->errorType = errorType;
    }

    virtual Return<void> onError(uint64_t deviceId, FaceError error, int32_t vendorCode) override {
        if ((this->filterErrors && this->errorType == error) || !this->filterErrors) {
            this->deviceId = deviceId;
            this->error = error;
            this->vendorCode = vendorCode;
            promise.set_value();
        }
        return Return<void>();
    }

    bool filterErrors;
    FaceError errorType;
    uint64_t deviceId;
    FaceError error;
    int32_t vendorCode;
    std::promise<void> promise;
};

class RemoveCallback : public FaceCallbackBase {
   public:
    RemoveCallback() {}

    virtual Return<void> onRemoved(uint64_t deviceId) override {
        promise.set_value();
        return Return<void>();
    }

    std::promise<void> promise;
};

// Test environment for Face Authentication HIDL HAL.
class FaceHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static FaceHidlEnvironment* Instance() {
        static FaceHidlEnvironment* instance = new FaceHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IBiometricsFace>(); }
};

class FaceHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   public:
    virtual void SetUp() override {
        mService = ::testing::VtsHalHidlTargetTestBase::getService<IBiometricsFace>(
            FaceHidlEnvironment::Instance()->getServiceName<IBiometricsFace>());
        ASSERT_FALSE(mService == nullptr);

        // Create an active group
        // Face service can only write to /data/vendor/users/*/faceAuthdata/ due to
        // SELinux Policy and Linux Dir Permissions
        Return<RequestStatus> res = mService->setActiveUser(kUserId, kTmpDir);
        ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));
    }

    virtual void TearDown() override {}

    sp<IBiometricsFace> mService;
};

// The service should be reachable.
TEST_F(FaceHidlTest, ConnectTest) {
    sp<FaceCallbackBase> cb = new FaceCallbackBase();
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));
}

// Starting the service with null callback should succeed.
TEST_F(FaceHidlTest, ConnectNullTest) {
    Return<uint64_t> rc = mService->setCallback(NULL);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));
}

// Pre-enroll should always return unique, cryptographically secure, non-zero number
TEST_F(FaceHidlTest, PreEnrollTest) {
    std::map<uint64_t, uint64_t> m;

    for (unsigned int i = 0; i < kIterations; ++i) {
        uint64_t res = static_cast<uint64_t>(mService->preEnroll());
        EXPECT_NE(0UL, res);
        m[res]++;
        EXPECT_EQ(1UL, m[res]);
    }
}

// Enroll with an invalid (all zeroes) HAT should fail.
TEST_F(FaceHidlTest, EnrollInvalidHatTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));

    uint8_t token[69];
    for (int i = 0; i < 69; i++) {
        token[i] = 0;
    }

    Return<RequestStatus> res = mService->enroll(token, kUserId, kTimeout);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // At least one call to onError should occur
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    ASSERT_NE(FaceError::ERROR_NO_ERROR, cb->error);
}

// Enroll with an invalid (null) HAT should fail.
TEST_F(FaceHidlTest, EnrollNullTest) {
    sp<ErrorCallback> cb = new ErrorCallback();
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));

    uint8_t token[69];
    Return<RequestStatus> res = mService->enroll(token, kUserId, kTimeout);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // At least one call to onError should occur
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    ASSERT_NE(FaceError::ERROR_NO_ERROR, cb->error);
}

// PostEnroll should always return within 3s
TEST_F(FaceHidlTest, PostEnrollTest) {
    sp<FaceCallbackBase> cb = new FaceCallbackBase();
    Return<uint64_t> rc = mService->setCallback(cb);

    auto start = std::chrono::system_clock::now();
    Return<RequestStatus> res = mService->postEnroll();
    auto elapsed = std::chrono::system_clock::now() - start;
    ASSERT_GE(kTimeoutInSeconds, elapsed);
}

// getAuthenticatorId should always return non-zero numbers
TEST_F(FaceHidlTest, GetAuthenticatorIdTest) {
    Return<uint64_t> res = mService->getAuthenticatorId();
    EXPECT_NE(0UL, static_cast<uint64_t>(res));
}

// Remove should succeed on any inputs
// At least one callback with "remaining=0" should occur
TEST_F(FaceHidlTest, RemoveFaceTest) {
    // Register callback
    sp<RemoveCallback> cb = new RemoveCallback(kUserId);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));

    // Remove face
    Return<RequestStatus> res = mService->remove(kUserId);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // A call to onRemove with should occur
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
}

// Active group should successfully set to a writable location.
TEST_F(FaceHidlTest, SetActiveUserTest) {
    // Create an active group
    Return<RequestStatus> res = mService->setActiveUser(2, kTmpDir);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // Reset active group
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));
}

// Active group should fail to set to an unwritable location.
TEST_F(FaceHidlTest, SetActiveUserUnwritableTest) {
    // Create an active group to an unwritable location (device root dir)
    Return<RequestStatus> res = mService->setActiveUser(3, "/");
    ASSERT_NE(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // Reset active group
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));
}

// Active group should fail to set to a null location.
TEST_F(FaceHidlTest, SetActiveUserNullTest) {
    // Create an active group to a null location.
    Return<RequestStatus> res = mService->setActiveUser(4, nullptr);
    ASSERT_NE(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // Reset active group
    res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));
}

// Cancel should always return ERROR_CANCELED from any starting state including
// the IDLE state.
TEST_F(FaceHidlTest, CancelTest) {
    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::ERROR_CANCELED);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0UL, static_cast<uint64_t>(rc));

    Return<RequestStatus> res = mService->cancel();
    // check that we were able to make an IPC request successfully
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));
    // check error should be ERROR_CANCELED
    ASSERT_EQ(FaceError::ERROR_CANCELED, cb->error);
}

// A call to cancel should succeed during enroll.
TEST_F(FaceHidlTest, CancelEnrollTest) {
    Return<RequestStatus> res = mService->setActiveUser(kUserId, kTmpDir);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::ERROR_CANCELED);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0U, static_cast<uint64_t>(rc));

    uint8_t token[69];
    res = mService->enroll(token, kUserId, kTimeout);
    // check that we were able to make an IPC request successfully
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    res = mService->cancel();
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));

    // check error should be ERROR_CANCELED
    ASSERT_EQ(FaceError::ERROR_CANCELED, cb->error);
}

// A call to cancel should succeed during authentication.
TEST_F(FaceHidlTest, CancelAuthTest) {
    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::ERROR_CANCELED);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0U, static_cast<uint64_t>(rc));

    Return<RequestStatus> res = mService->authenticate(0, kUserId);
    // check that we were able to make an IPC request successfully
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    res = mService->cancel();
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));

    // check error should be ERROR_CANCELED
    ASSERT_EQ(FaceError::ERROR_CANCELED, cb->error);
}

// A call to cancel should succeed during removal.
TEST_F(FaceHidlTest, CancelRemoveTest) {
    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::ERROR_CANCELED);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0U, static_cast<uint64_t>(rc));

    // Remove a face
    Return<RequestStatus> res = mService->remove(kUserId, 1);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    res = mService->cancel();
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));

    // check error should be ERROR_CANCELED
    ASSERT_EQ(FaceError::ERROR_CANCELED, cb->error);
}

// A call to cancel should succeed during all faces removal.
TEST_F(FaceHidlTest, CancelRemoveAllTest) {
    sp<ErrorCallback> cb = new ErrorCallback(true, FaceError::ERROR_CANCELED);
    Return<uint64_t> rc = mService->setCallback(cb);
    ASSERT_NE(0U, static_cast<uint64_t>(rc));

    // Remove a face
    Return<RequestStatus> res = mService->remove(kUserId, 0);
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    res = mService->cancel();
    ASSERT_EQ(RequestStatus::SYS_OK, static_cast<RequestStatus>(res));

    // make sure callback was invoked within kTimeoutInSeconds
    ASSERT_TRUE(waitForCallback(cb->promise.get_future()));

    // check error should be ERROR_CANCELED
    ASSERT_EQ(FaceError::ERROR_CANCELED, cb->error);
}
}  // anonymous namespace

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(FaceHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    FaceHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    LOG(INFO) << "Test result = " << status;
    return status;
}
