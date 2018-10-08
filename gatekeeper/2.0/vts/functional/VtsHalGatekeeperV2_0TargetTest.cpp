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

#define LOG_TAG "gatekeeper_hidl_hal_test"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <inttypes.h>
#include <unistd.h>

#include <hardware/hw_auth_token.h>

#include <android/hardware/gatekeeper/1.0/types.h>
#include <android/hardware/gatekeeper/2.0/IGatekeeper.h>
#include <android/log.h>

#include <log/log.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

using ::android::sp;
using ::android::hardware::gatekeeper::V1_0::GatekeeperStatusCode;
using ::android::hardware::keymaster_capability::V1_0::KeymasterCapability;

namespace android::hardware::gatekeeper::V2_0::test {

struct GatekeeperRequest {
    uint32_t uid;
    uint64_t challenge;
    hidl_vec<uint8_t> curPwdHandle;
    hidl_vec<uint8_t> curPwd;
    hidl_vec<uint8_t> newPwd;
    GatekeeperRequest() : uid(0), challenge(0) {}
};

using EnrollResponse = std::tuple<GatekeeperStatusCode, uint32_t, hidl_vec<uint8_t>>;
using VerifyResponse = std::tuple<GatekeeperStatusCode, uint32_t, KeymasterCapability>;

// Test environment for Gatekeeper HIDL HAL.
class GatekeeperHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
   public:
    // get the test environment singleton
    static GatekeeperHidlEnvironment* Instance() {
        static GatekeeperHidlEnvironment* instance = new GatekeeperHidlEnvironment;
        return instance;
    }

    virtual void registerTestServices() override { registerTestService<IGatekeeper>(); }

   private:
    GatekeeperHidlEnvironment() {}
};

// The main test class for Gatekeeper HIDL HAL.
class GatekeeperHidlTest : public ::testing::VtsHalHidlTargetTestBase {
   protected:
    void setUid(uint32_t uid) { uid_ = uid; }

    EnrollResponse doEnroll(GatekeeperRequest& req) {
        while (true) {
            EnrollResponse retval;

            auto ret = gatekeeper_->enroll(uid_, req.curPwdHandle, req.curPwd, req.newPwd,
                                           [&](GatekeeperStatusCode code, uint32_t timeout,
                                               const hidl_vec<uint8_t>& newPasswordHandle) {
                                               retval =
                                                   EnrollResponse(code, timeout, newPasswordHandle);
                                           });
            EXPECT_TRUE(ret.isOk());

            if (std::get<0>(retval) != GatekeeperStatusCode::ERROR_RETRY_TIMEOUT) {
                return retval;
            }
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
    }

    VerifyResponse doVerify(GatekeeperRequest& req) {
        while (true) {
            VerifyResponse retval;

            auto ret = gatekeeper_->verify(uid_, req.challenge, req.curPwdHandle, req.newPwd,
                                           [&](GatekeeperStatusCode code, uint32_t timeout,
                                               const KeymasterCapability& capability) {
                                               retval = VerifyResponse(code, timeout, capability);
                                           });
            EXPECT_TRUE(ret.isOk());
            if (std::get<0>(retval) != GatekeeperStatusCode::ERROR_RETRY_TIMEOUT) {
                return retval;
            }
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
    }

    GatekeeperStatusCode doDeleteUser() {
        while (true) {
            auto code = gatekeeper_->deleteUser(uid_);
            EXPECT_TRUE(code.isOk());
            if (code != GatekeeperStatusCode::ERROR_RETRY_TIMEOUT) return code;
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
    }

    GatekeeperStatusCode doDeleteAllUsers() {
        while (true) {
            auto code = gatekeeper_->deleteAllUsers();
            EXPECT_TRUE(code.isOk());
            if (code != GatekeeperStatusCode::ERROR_RETRY_TIMEOUT) return code;
            ALOGI("%s: got retry code; retrying in 1 sec", __func__);
            sleep(1);
        }
    }

    hidl_vec<uint8_t> generatePassword(uint8_t seed) {
        hidl_vec<uint8_t> password;
        password.resize(16);
        memset(password.data(), seed, password.size());
        return password;
    }

    void checkEnroll(const EnrollResponse& rsp, bool expectSuccess) {
        auto [code, timeout, newHandle] = rsp;

        if (expectSuccess) {
            EXPECT_EQ(GatekeeperStatusCode::STATUS_OK, code);
            EXPECT_NE(nullptr, newHandle.data());
            EXPECT_GT(newHandle.size(), UINT32_C(0));
        } else {
            EXPECT_EQ(GatekeeperStatusCode::ERROR_GENERAL_FAILURE, code);
            EXPECT_EQ(UINT32_C(0), newHandle.size());
        }
    }

    void checkVerify(const VerifyResponse& rsp, uint64_t challenge, bool expectSuccess) {
        auto [code, timeout, capability] = rsp;

        if (expectSuccess) {
            EXPECT_GE(code, GatekeeperStatusCode::STATUS_OK);
            EXPECT_LE(code, GatekeeperStatusCode::STATUS_REENROLL);
            EXPECT_EQ(challenge, capability.challenge);
        } else {
            EXPECT_EQ(GatekeeperStatusCode::ERROR_GENERAL_FAILURE, code);
            EXPECT_EQ(UINT32_C(0), capability.secure_token.size());
        }
    }

    EnrollResponse enrollNewPassword(const hidl_vec<uint8_t>& password, bool expectSuccess) {
        GatekeeperRequest req;
        req.newPwd = password;
        auto rsp = doEnroll(req);
        checkEnroll(rsp, expectSuccess);
        return rsp;
    }

    VerifyResponse verifyPassword(const hidl_vec<uint8_t>& password,
                                  const hidl_vec<uint8_t>& passwordHandle, uint64_t challenge,
                                  bool expectSuccess) {
        GatekeeperRequest verifyReq;

        // build verify request for the same password (we want it to succeed)
        verifyReq.newPwd = password;
        // use enrolled password handle we've got
        verifyReq.curPwdHandle = passwordHandle;
        verifyReq.challenge = challenge;
        auto verifyRsp = doVerify(verifyReq);
        checkVerify(verifyRsp, challenge, expectSuccess);
        return verifyRsp;
    }

   protected:
    sp<IGatekeeper> gatekeeper_;
    uint32_t uid_;

   public:
    GatekeeperHidlTest() : uid_(0) {}
    virtual void SetUp() override {
        gatekeeper_ = ::testing::VtsHalHidlTargetTestBase::getService<IGatekeeper>(
            GatekeeperHidlEnvironment::Instance()->getServiceName<IGatekeeper>());
        ASSERT_NE(nullptr, gatekeeper_.get());
        doDeleteAllUsers();
    }

    virtual void TearDown() override { doDeleteAllUsers(); }
};

/**
 * Ensure we can enroll new password
 */
TEST_F(GatekeeperHidlTest, EnrollSuccess) {
    ALOGI("Testing Enroll (expected success)");
    auto password = generatePassword(0);
    enrollNewPassword(password, true);
    ALOGI("Testing Enroll done");
}

/**
 * Ensure we can not enroll empty password
 */
TEST_F(GatekeeperHidlTest, EnrollNoPassword) {
    hidl_vec<uint8_t> password;
    ALOGI("Testing Enroll (expected failure)");
    enrollNewPassword(password, false);
    ALOGI("Testing Enroll done");
}

/**
 * Ensure we can successfully verify previously enrolled password
 */
TEST_F(GatekeeperHidlTest, VerifySuccess) {
    ALOGI("Testing Enroll+Verify (expected success)");
    auto password = generatePassword(0);
    auto enrollRsp = enrollNewPassword(password, true);
    verifyPassword(password, std::get<2>(enrollRsp), 1, true);
    ALOGI("Testing Enroll+Verify done");
}

/**
 * Ensure we can securely update password (keep the same
 * secure user_id) if we prove we know old password
 */
TEST_F(GatekeeperHidlTest, TrustedReenroll) {
    auto password = generatePassword(0);

    ALOGI("Testing Trusted Reenroll (expected success)");
    auto enrollRsp = enrollNewPassword(password, true);
    auto verifyRsp = verifyPassword(password, std::get<2>(enrollRsp), 0, true);
    ALOGI("Primary Enroll+Verify done");

    // Wait a millisecond to be sure that the second verification is at a different timestamp.
    usleep(1000);

    auto newPassword = generatePassword(1);
    GatekeeperRequest reenrollReq;
    reenrollReq.newPwd = newPassword;
    reenrollReq.curPwd = password;
    reenrollReq.curPwdHandle = std::get<2>(enrollRsp);

    auto reenrollRsp = doEnroll(reenrollReq);
    checkEnroll(reenrollRsp, true);
    auto reenrollVerifyRsp = verifyPassword(newPassword, std::get<2>(reenrollRsp), 0, true);
    ALOGI("Trusted ReEnroll+Verify done");

    auto& verifyCap = std::get<2>(verifyRsp);
    auto& reverifyCap = std::get<2>(reenrollVerifyRsp);

    EXPECT_EQ(verifyCap.ids.size(), 1U);
    EXPECT_EQ(verifyCap.ids, reverifyCap.ids);
    EXPECT_NE(verifyCap.secure_token, reverifyCap.secure_token);
    ALOGI("Testing Trusted Reenroll done");
}

/**
 * Ensure we can update password (and get new
 * secure user_id) if we don't know old password
 */
TEST_F(GatekeeperHidlTest, UntrustedReenroll) {
    ALOGI("Testing Untrusted Reenroll (expected success)");
    auto password = generatePassword(0);
    auto enrollRsp = enrollNewPassword(password, true);
    auto verifyRsp = verifyPassword(password, std::get<2>(enrollRsp), 0, true);
    ALOGI("Primary Enroll+Verify done");

    auto newPassword = generatePassword(1);
    auto reenrollRsp = enrollNewPassword(newPassword, true);
    auto reenrollVerifyRsp = verifyPassword(newPassword, std::get<2>(reenrollRsp), 0, true);
    ALOGI("Untrusted ReEnroll+Verify done");

    auto& verifyCap = std::get<2>(verifyRsp);
    auto& reverifyCap = std::get<2>(reenrollVerifyRsp);

    EXPECT_EQ(verifyCap.ids.size(), 1U);
    EXPECT_EQ(reverifyCap.ids.size(), 1U);
    EXPECT_NE(verifyCap.ids, reverifyCap.ids);
    EXPECT_NE(verifyCap.secure_token, reverifyCap.secure_token);
    ALOGI("Testing Untrusted Reenroll done");
}

/**
 * Ensure we dont get successful verify with invalid data
 */
TEST_F(GatekeeperHidlTest, VerifyNoData) {
    hidl_vec<uint8_t> password;
    hidl_vec<uint8_t> passwordHandle;

    ALOGI("Testing Verify (expected failure)");
    auto [code, timeout, cap] = verifyPassword(password, passwordHandle, 0, false);
    EXPECT_EQ(GatekeeperStatusCode::ERROR_GENERAL_FAILURE, code);
    ALOGI("Testing Verify done");
}

/**
 * Ensure we can not verify password after we enrolled it and then deleted user
 */
TEST_F(GatekeeperHidlTest, DeleteUserTest) {
    ALOGI("Testing deleteUser (expected success)");
    setUid(10001);
    auto password = generatePassword(0);
    auto enrollRsp = enrollNewPassword(password, true);
    auto verifyRsp = verifyPassword(password, std::get<2>(enrollRsp), 0, true);
    ALOGI("Enroll+Verify done");

    auto delRsp = doDeleteUser();

    EXPECT_TRUE(delRsp == GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED ||
                delRsp == GatekeeperStatusCode::STATUS_OK);
    ALOGI("DeleteUser done");
    if (delRsp == GatekeeperStatusCode::STATUS_OK) {
        auto verifyRsp = verifyPassword(password, std::get<2>(enrollRsp), 0, false);
        EXPECT_EQ(GatekeeperStatusCode::ERROR_GENERAL_FAILURE, std::get<0>(verifyRsp));
        ALOGI("Verify after Delete done (must fail)");
    }
    ALOGI("Testing deleteUser done: rsp=%" PRIi32, delRsp);
}

/**
 * Ensure we can not delete a user that does not exist
 */
TEST_F(GatekeeperHidlTest, DeleteInvalidUserTest) {
    ALOGI("Testing deleteUser (expected failure)");
    setUid(10002);
    auto password = generatePassword(0);
    auto enrollRsp = enrollNewPassword(password, true);
    auto verifyRsp = verifyPassword(password, std::get<2>(enrollRsp), 0, true);
    ALOGI("Enroll+Verify done");

    // Delete the user
    auto delRsp1 = doDeleteUser();
    EXPECT_TRUE(delRsp1 == GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED ||
                delRsp1 == GatekeeperStatusCode::STATUS_OK);

    // Delete the user again
    auto delRsp2 = doDeleteUser();
    EXPECT_TRUE(delRsp2 == GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED ||
                delRsp2 == GatekeeperStatusCode::ERROR_GENERAL_FAILURE);
    ALOGI("DeleteUser done");
    ALOGI("Testing deleteUser done: rsp=%" PRIi32, delRsp2);
}

/**
 * Ensure we can not verify passwords after we enrolled them and then deleted
 * all users
 */
TEST_F(GatekeeperHidlTest, DeleteAllUsersTest) {
    struct UserData {
        uint32_t userId;
        hidl_vec<uint8_t> password;
        EnrollResponse enrollRsp;
        VerifyResponse verifyRsp;
        UserData(int id) { userId = id; }
    } users[3]{10001, 10002, 10003};
    ALOGI("Testing deleteAllUsers (expected success)");

    // enroll multiple users
    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        setUid(users[i].userId);
        users[i].password = generatePassword((i % 255) + 1);
        users[i].enrollRsp = enrollNewPassword(users[i].password, true);
    }
    ALOGI("Multiple users enrolled");

    // verify multiple users
    for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
        setUid(users[i].userId);
        users[i].verifyRsp =
            verifyPassword(users[i].password, std::get<2>(users[i].enrollRsp), 0, true);
    }
    ALOGI("Multiple users verified");

    auto delAllRsp = doDeleteAllUsers();
    EXPECT_TRUE(delAllRsp == GatekeeperStatusCode::ERROR_NOT_IMPLEMENTED ||
                delAllRsp == GatekeeperStatusCode::STATUS_OK);
    ALOGI("All users deleted");

    if (delAllRsp == GatekeeperStatusCode::STATUS_OK) {
        // verify multiple users after they are deleted; all must fail
        for (size_t i = 0; i < sizeof(users) / sizeof(users[0]); ++i) {
            setUid(users[i].userId);
            users[i].verifyRsp =
                verifyPassword(users[i].password, std::get<2>(users[i].enrollRsp), 0, false);
            EXPECT_EQ(GatekeeperStatusCode::ERROR_GENERAL_FAILURE, std::get<0>(users[i].verifyRsp));
        }
        ALOGI("Multiple users verified after delete (all must fail)");
    }

    ALOGI("Testing deleteAllUsers done: rsp=%" PRIi32, delAllRsp);
}

int main(int argc, char** argv) {
    ::testing::AddGlobalTestEnvironment(GatekeeperHidlEnvironment::Instance());
    ::testing::InitGoogleTest(&argc, argv);
    GatekeeperHidlEnvironment::Instance()->init(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}

}  // namespace android::hardware::gatekeeper::V2_0::test
