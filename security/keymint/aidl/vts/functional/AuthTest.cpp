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

#define LOG_TAG "keymint_1_test"
#include <cutils/log.h>

#include <iostream>
#include <optional>

#include "KeyMintAidlTestBase.h"

#include <aidl/android/hardware/gatekeeper/GatekeeperEnrollResponse.h>
#include <aidl/android/hardware/gatekeeper/GatekeeperVerifyResponse.h>
#include <aidl/android/hardware/gatekeeper/IGatekeeper.h>
#include <aidl/android/hardware/security/secureclock/ISecureClock.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>

using aidl::android::hardware::gatekeeper::GatekeeperEnrollResponse;
using aidl::android::hardware::gatekeeper::GatekeeperVerifyResponse;
using aidl::android::hardware::gatekeeper::IGatekeeper;
using aidl::android::hardware::security::keymint::HardwareAuthToken;
using aidl::android::hardware::security::secureclock::ISecureClock;

namespace aidl::android::hardware::security::keymint::test {

class AuthTest : public KeyMintAidlTestBase {
  public:
    void SetUp() {
        KeyMintAidlTestBase::SetUp();

        // Find the default Gatekeeper instance.
        string gk_name = string(IGatekeeper::descriptor) + "/default";
        if (AServiceManager_isDeclared(gk_name.c_str())) {
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(gk_name.c_str()));
            gk_ = IGatekeeper::fromBinder(binder);
        } else {
            // Prior to Android U, Gatekeeper was HIDL not AIDL and so may not be present.
            std::cerr << "No AIDL Gatekeeper instance for '" << gk_name << "' found.\n";
        }

        // If the device needs timestamps, find the default ISecureClock instance.
        if (timestamp_token_required_) {
            string clock_name = string(ISecureClock::descriptor) + "/default";
            if (AServiceManager_isDeclared(clock_name.c_str())) {
                ::ndk::SpAIBinder binder(AServiceManager_waitForService(clock_name.c_str()));
                clock_ = ISecureClock::fromBinder(binder);
            } else {
                std::cerr << "No ISecureClock instance for '" << clock_name << "' found.\n";
            }
        }

        // Enroll a user
        uid_ = 10001;
        password_ = "hunter2";
        std::optional<GatekeeperEnrollResponse> rsp = doEnroll(password_);
        ASSERT_TRUE(rsp.has_value());
        sid_ = rsp->secureUserId;
        handle_ = rsp->data;
    }

    void TearDown() {
        if (gk_ == nullptr) return;
        gk_->deleteUser(uid_);
    }

    std::optional<GatekeeperEnrollResponse> doEnroll(const std::vector<uint8_t>& newPwd,
                                                     const std::vector<uint8_t>& curHandle = {},
                                                     const std::vector<uint8_t>& curPwd = {}) {
        while (true) {
            GatekeeperEnrollResponse rsp;
            Status status = gk_->enroll(uid_, curHandle, curPwd, newPwd, &rsp);
            if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                status.getServiceSpecificError() == IGatekeeper::ERROR_RETRY_TIMEOUT) {
                sleep(1);
                continue;
            }
            if (status.isOk()) {
                return std::move(rsp);
            } else {
                GTEST_LOG_(ERROR) << "doEnroll failed: " << status;
                return std::nullopt;
            }
        }
    }
    std::optional<GatekeeperEnrollResponse> doEnroll(const string& newPwd,
                                                     const std::vector<uint8_t> curHandle = {},
                                                     const string& curPwd = {}) {
        return doEnroll(std::vector<uint8_t>(newPwd.begin(), newPwd.end()), curHandle,
                        std::vector<uint8_t>(curPwd.begin(), curPwd.end()));
    }

    std::optional<HardwareAuthToken> doVerify(uint64_t challenge,
                                              const std::vector<uint8_t>& handle,
                                              const std::vector<uint8_t>& pwd) {
        while (true) {
            GatekeeperVerifyResponse rsp;
            Status status = gk_->verify(uid_, challenge, handle, pwd, &rsp);
            if (!status.isOk() && status.getExceptionCode() == EX_SERVICE_SPECIFIC &&
                status.getServiceSpecificError() == IGatekeeper::ERROR_RETRY_TIMEOUT) {
                sleep(1);
                continue;
            }
            if (status.isOk()) {
                return rsp.hardwareAuthToken;
            } else {
                GTEST_LOG_(ERROR) << "doVerify failed: " << status;
                return std::nullopt;
            }
        }
    }
    std::optional<HardwareAuthToken> doVerify(uint64_t challenge,
                                              const std::vector<uint8_t>& handle,
                                              const string& pwd) {
        return doVerify(challenge, handle, std::vector<uint8_t>(pwd.begin(), pwd.end()));
    }

    // Variants of the base class methods but with authentication information included.
    string ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                          const string& message, const AuthorizationSet& in_params,
                          AuthorizationSet* out_params, const HardwareAuthToken& hat) {
        AuthorizationSet begin_out_params;
        ErrorCode result = Begin(operation, key_blob, in_params, out_params, hat);
        EXPECT_EQ(ErrorCode::OK, result);
        if (result != ErrorCode::OK) {
            return "";
        }

        std::optional<secureclock::TimeStampToken> time_token = std::nullopt;
        if (timestamp_token_required_ && clock_ != nullptr) {
            // Ask a secure clock instance for a timestamp, including the per-op challenge.
            secureclock::TimeStampToken token;
            EXPECT_EQ(ErrorCode::OK,
                      GetReturnErrorCode(clock_->generateTimeStamp(challenge_, &token)));
            time_token = token;
        }

        string output;
        EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &output, hat, time_token));
        return output;
    }

    string EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params,
                          const HardwareAuthToken& hat) {
        SCOPED_TRACE("EncryptMessage");
        return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params, hat);
    }

    string DecryptMessage(const vector<uint8_t>& key_blob, const string& ciphertext,
                          const AuthorizationSet& params, const HardwareAuthToken& hat) {
        SCOPED_TRACE("DecryptMessage");
        AuthorizationSet out_params;
        string plaintext =
                ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params, hat);
        EXPECT_TRUE(out_params.empty());
        return plaintext;
    }

  protected:
    std::shared_ptr<IGatekeeper> gk_;
    std::shared_ptr<ISecureClock> clock_;
    string password_;
    uint32_t uid_;
    long sid_;
    std::vector<uint8_t> handle_;
};

// Test use of a key that requires user-authentication within recent history.
TEST_P(AuthTest, TimeoutAuthentication) {
    if (gk_ == nullptr) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (timestamp_token_required_ && clock_ == nullptr) {
        GTEST_SKIP() << "Device requires timestamps and no ISecureClock available";
    }

    // Create an AES key that requires authentication within the last 3 seconds.
    uint32_t timeout_secs = 3;
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD)
                           .Authorization(TAG_AUTH_TIMEOUT, timeout_secs);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Attempt to use the AES key without authentication.
    string message = "Hello World!";
    AuthorizationSet out_params;
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));

    // Verify to get a HAT, arbitrary challenge.
    uint64_t challenge = 42;
    std::optional<HardwareAuthToken> hat = doVerify(challenge, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);

    // Adding the auth token makes it possible to use the AES key.
    string ciphertext = EncryptMessage(keyblob, message, params, &out_params, hat.value());
    string plaintext = DecryptMessage(keyblob, ciphertext, params, hat.value());
    EXPECT_EQ(message, plaintext);

    // Altering a single bit in the MAC means no auth.
    HardwareAuthToken dodgy_hat = hat.value();
    ASSERT_GT(dodgy_hat.mac.size(), 0);
    dodgy_hat.mac[0] ^= 0x01;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, dodgy_hat));

    // Wait for long enough that the token expires.
    sleep(timeout_secs + 1);
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, hat));
}

// Test use of a key that requires an auth token for each action on the operation, with
// a per-operation challenge value included.
TEST_P(AuthTest, AuthPerOperation) {
    if (gk_ == nullptr) {
        GTEST_SKIP() << "No Gatekeeper available";
    }
    if (timestamp_token_required_ && clock_ == nullptr) {
        GTEST_SKIP() << "Device requires timestamps and no ISecureClock available";
    }

    // Create an AES key that requires authentication per-action.
    auto builder = AuthorizationSetBuilder()
                           .AesEncryptionKey(256)
                           .BlockMode(BlockMode::ECB)
                           .Padding(PaddingMode::PKCS7)
                           .Authorization(TAG_USER_SECURE_ID, sid_)
                           .Authorization(TAG_USER_AUTH_TYPE, HardwareAuthenticatorType::PASSWORD);
    vector<uint8_t> keyblob;
    vector<KeyCharacteristics> key_characteristics;
    vector<Certificate> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              GenerateKey(builder, std::nullopt, &keyblob, &key_characteristics, &cert_chain));

    // Attempt to use the AES key without authentication fails after begin.
    string message = "Hello World!";
    AuthorizationSet out_params;
    auto params = AuthorizationSetBuilder().BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    string output;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED, Finish(message, {} /* signature */, &output));

    // Verify to get a HAT, but with an arbitrary challenge.
    uint64_t unrelated_challenge = 42;
    std::optional<HardwareAuthToken> unrelated_hat =
            doVerify(unrelated_challenge, handle_, password_);
    ASSERT_TRUE(unrelated_hat.has_value());
    EXPECT_EQ(unrelated_hat->userId, sid_);

    // Attempt to use the AES key with an unrelated authentication fails after begin.
    EXPECT_EQ(ErrorCode::OK,
              Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params, unrelated_hat.value()));
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &output, unrelated_hat.value()));

    // Now get a HAT with the challenge from an in-progress operation.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    std::optional<HardwareAuthToken> hat = doVerify(challenge_, handle_, password_);
    ASSERT_TRUE(hat.has_value());
    EXPECT_EQ(hat->userId, sid_);
    string ciphertext;
    EXPECT_EQ(ErrorCode::OK, Finish(message, {} /* signature */, &ciphertext, hat.value()));

    // Altering a single bit in the MAC means no auth.
    EXPECT_EQ(ErrorCode::OK, Begin(KeyPurpose::ENCRYPT, keyblob, params, &out_params));
    std::optional<HardwareAuthToken> dodgy_hat = doVerify(challenge_, handle_, password_);
    ASSERT_TRUE(dodgy_hat.has_value());
    EXPECT_EQ(dodgy_hat->userId, sid_);
    ASSERT_GT(dodgy_hat->mac.size(), 0);
    dodgy_hat->mac[0] ^= 0x01;
    EXPECT_EQ(ErrorCode::KEY_USER_NOT_AUTHENTICATED,
              Finish(message, {} /* signature */, &ciphertext, hat.value()));
}

INSTANTIATE_KEYMINT_AIDL_TEST(AuthTest);

}  // namespace aidl::android::hardware::security::keymint::test
