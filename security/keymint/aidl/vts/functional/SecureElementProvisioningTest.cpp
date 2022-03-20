/*
 * Copyright (C) 2021 The Android Open Source Project
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

#define LOG_TAG "keymint_2_se_provisioning_test"

#include <map>
#include <memory>
#include <vector>

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <cppbor_parse.h>
#include <keymaster/cppcose/cppcose.h>
#include <keymint_support/key_param_output.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using std::array;
using std::map;
using std::shared_ptr;
using std::vector;

class SecureElementProvisioningTest : public testing::Test {
  protected:
    static void SetUpTestSuite() {
        auto params = ::android::getAidlHalInstanceNames(IKeyMintDevice::descriptor);
        for (auto& param : params) {
            ASSERT_TRUE(AServiceManager_isDeclared(param.c_str()))
                    << "IKeyMintDevice instance " << param << " found but not declared.";
            ::ndk::SpAIBinder binder(AServiceManager_waitForService(param.c_str()));
            auto keymint = IKeyMintDevice::fromBinder(binder);
            ASSERT_NE(keymint, nullptr) << "Failed to get IKeyMintDevice instance " << param;

            KeyMintHardwareInfo info;
            ASSERT_TRUE(keymint->getHardwareInfo(&info).isOk());
            ASSERT_EQ(keymints_.count(info.securityLevel), 0)
                    << "There must be exactly one IKeyMintDevice with security level "
                    << info.securityLevel;

            keymints_[info.securityLevel] = std::move(keymint);
        }
    }

    void validateMacedRootOfTrust(const vector<uint8_t>& rootOfTrust) {
        const auto [macItem, macEndPos, macErrMsg] = cppbor::parse(rootOfTrust);
        ASSERT_TRUE(macItem) << "Root of trust parsing failed: " << macErrMsg;
        ASSERT_EQ(macItem->semanticTagCount(), 1) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(macItem->semanticTag(0), cppcose::kCoseMac0SemanticTag)
                << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(macItem->asArray()) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(macItem->asArray()->size(), 4) << "RoT: " << bin2hex(rootOfTrust);

        const auto& protectedItem = macItem->asArray()->get(0);
        ASSERT_TRUE(protectedItem) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(protectedItem->asBstr()) << "RoT: " << bin2hex(rootOfTrust);
        const auto [protMap, protEndPos, protErrMsg] = cppbor::parse(protectedItem->asBstr());
        ASSERT_TRUE(protMap) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(protMap->asMap()) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(protMap->asMap()->size(), 1) << "RoT: " << bin2hex(rootOfTrust);

        const auto& algorithm = protMap->asMap()->get(1 /* Algorithm */);
        ASSERT_TRUE(algorithm) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(algorithm->asInt()) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(algorithm->asInt()->value(), 5 /* HMAC-SHA-256 */)
                << "RoT: " << bin2hex(rootOfTrust);

        const auto& unprotItem = macItem->asArray()->get(1);
        ASSERT_TRUE(unprotItem) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(unprotItem->asMap()) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(unprotItem->asMap()->size(), 0) << "RoT: " << bin2hex(rootOfTrust);

        const auto& payload = macItem->asArray()->get(2);
        ASSERT_TRUE(payload) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(payload->asBstr()) << "RoT: " << bin2hex(rootOfTrust);
        validateRootOfTrust(payload->asBstr()->value());

        const auto& tag = macItem->asArray()->get(3);
        ASSERT_TRUE(tag) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_TRUE(tag->asBstr()) << "RoT: " << bin2hex(rootOfTrust);
        ASSERT_EQ(tag->asBstr()->value().size(), 32) << "RoT: " << bin2hex(rootOfTrust);
        // Cannot validate tag correctness.  Only the secure side has the necessary key.
    }

    void validateRootOfTrust(const vector<uint8_t>& payload) {
        const auto [rot, rotPos, rotErrMsg] = cppbor::parse(payload);
        ASSERT_TRUE(rot) << "RoT payload: " << bin2hex(payload);
        ASSERT_EQ(rot->semanticTagCount(), 1) << "RoT payload: " << bin2hex(payload);
        ASSERT_EQ(rot->semanticTag(), 1000) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(rot->asArray()) << "RoT payload: " << bin2hex(payload);

        const auto& vbKey = rot->asArray()->get(0);
        ASSERT_TRUE(vbKey) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(vbKey->asBstr()) << "RoT payload: " << bin2hex(payload);
        ASSERT_EQ(vbKey->asBstr()->value().size(), 32) << "RoT payload: " << bin2hex(payload);
        // TODO validate vbKey content

        const auto& deviceLocked = rot->asArray()->get(1);
        ASSERT_TRUE(deviceLocked) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(deviceLocked->asBool()) << "RoT payload: " << bin2hex(payload);
        // TODO validate deviceLocked content.

        const auto& verifiedBootState = rot->asArray()->get(2);
        ASSERT_TRUE(verifiedBootState) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(verifiedBootState->asInt()) << "RoT payload: " << bin2hex(payload);
        // TODO validate verified boot state.

        const auto& verifiedBootHash = rot->asArray()->get(3);
        ASSERT_TRUE(verifiedBootHash) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(verifiedBootHash->asBstr()) << "RoT payload: " << bin2hex(payload);
        ASSERT_EQ(verifiedBootHash->asBstr()->value().size(), 32)
                << "RoT payload: " << bin2hex(payload);
        // TODO validate verifiedBootHash content.

        const auto& bootPatchLevel = rot->asArray()->get(4);
        ASSERT_TRUE(bootPatchLevel) << "RoT payload: " << bin2hex(payload);
        ASSERT_TRUE(bootPatchLevel->asInt()) << "RoT payload: " << bin2hex(payload);
        // TODO validate bootPatchLevel value.
    }

    int32_t AidlVersion(shared_ptr<IKeyMintDevice> keymint) {
        int32_t version = 0;
        auto status = keymint->getInterfaceVersion(&version);
        if (!status.isOk()) {
            ADD_FAILURE() << "Failed to determine interface version";
        }
        return version;
    }

    static map<SecurityLevel, shared_ptr<IKeyMintDevice>> keymints_;
};

map<SecurityLevel, shared_ptr<IKeyMintDevice>> SecureElementProvisioningTest::keymints_;

TEST_F(SecureElementProvisioningTest, ValidConfigurations) {
    if (keymints_.empty()) {
        GTEST_SKIP() << "Test not applicable to device with no KeyMint devices";
    }
    // TEE is required
    ASSERT_EQ(keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT), 1);
    // StrongBox is optional
    ASSERT_LE(keymints_.count(SecurityLevel::STRONGBOX), 1);
}

TEST_F(SecureElementProvisioningTest, TeeOnly) {
    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    // Execute the test only for KeyMint version >= 2.
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge1 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    array<uint8_t, 16> challenge2 = {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    vector<uint8_t> rootOfTrust1;
    Status result = tee->getRootOfTrust(challenge1, &rootOfTrust1);
    ASSERT_TRUE(result.isOk()) << "getRootOfTrust returned " << result.getServiceSpecificError();
    validateMacedRootOfTrust(rootOfTrust1);

    vector<uint8_t> rootOfTrust2;
    result = tee->getRootOfTrust(challenge2, &rootOfTrust2);
    ASSERT_TRUE(result.isOk());
    validateMacedRootOfTrust(rootOfTrust2);
    ASSERT_NE(rootOfTrust1, rootOfTrust2);

    vector<uint8_t> rootOfTrust3;
    result = tee->getRootOfTrust(challenge1, &rootOfTrust3);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(rootOfTrust1, rootOfTrust3);
}

TEST_F(SecureElementProvisioningTest, TeeDoesNotImplementStrongBoxMethods) {
    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    // Execute the test only for KeyMint version >= 2.
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = tee->getRootOfTrustChallenge(&challenge);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);

    result = tee->sendRootOfTrust({});
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);
}

TEST_F(SecureElementProvisioningTest, StrongBoxDoesNotImplementTeeMethods) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    vector<uint8_t> rootOfTrust;
    Status result = sb->getRootOfTrust({}, &rootOfTrust);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()), ErrorCode::UNIMPLEMENTED);
}

TEST_F(SecureElementProvisioningTest, UnimplementedTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) {
        // Strongbox does not have to implement this feature if it has uses an alternative mechanism
        // to provision the root of trust.  In that case it MUST return UNIMPLEMENTED, both from
        // getRootOfTrustChallenge() and from sendRootOfTrust().
        ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
                  ErrorCode::UNIMPLEMENTED);

        result = sb->sendRootOfTrust({});
        ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
        ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
                  ErrorCode::UNIMPLEMENTED);

        SUCCEED() << "This Strongbox implementation does not use late root of trust delivery.";
        return;
    }
}

TEST_F(SecureElementProvisioningTest, ChallengeQualityTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    array<uint8_t, 16> challenge1;
    Status result = sb->getRootOfTrustChallenge(&challenge1);
    if (!result.isOk()) return;

    array<uint8_t, 16> challenge2;
    result = sb->getRootOfTrustChallenge(&challenge2);
    ASSERT_TRUE(result.isOk());
    ASSERT_NE(challenge1, challenge2);

    // TODO: When we add entropy testing in other relevant places in these tests, add it here, too,
    // to verify that challenges appear to have adequate entropy.
}

TEST_F(SecureElementProvisioningTest, ProvisioningTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) return;

    vector<uint8_t> rootOfTrust;
    result = tee->getRootOfTrust(challenge, &rootOfTrust);
    ASSERT_TRUE(result.isOk());

    validateMacedRootOfTrust(rootOfTrust);

    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_TRUE(result.isOk());

    // Sending again must fail, because a new challenge is required.
    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_FALSE(result.isOk());
}

TEST_F(SecureElementProvisioningTest, InvalidProvisioningTest) {
    if (keymints_.count(SecurityLevel::STRONGBOX) == 0) {
        // Need a StrongBox to provision.
        GTEST_SKIP() << "Test not applicable to device with no StrongBox KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto sb = keymints_.find(SecurityLevel::STRONGBOX)->second;
    if (AidlVersion(sb) < 2) {
        GTEST_SKIP() << "Test not applicable to StrongBox KeyMint device before v2";
    }

    if (keymints_.count(SecurityLevel::TRUSTED_ENVIRONMENT) == 0) {
        GTEST_SKIP() << "Test not applicable to device with no TEE KeyMint device";
    }
    // Execute the test only for KeyMint version >= 2.
    auto tee = keymints_.find(SecurityLevel::TRUSTED_ENVIRONMENT)->second;
    if (AidlVersion(tee) < 2) {
        GTEST_SKIP() << "Test not applicable to TEE KeyMint device before v2";
    }

    array<uint8_t, 16> challenge;
    Status result = sb->getRootOfTrustChallenge(&challenge);
    if (!result.isOk()) return;

    result = sb->sendRootOfTrust({});
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
              ErrorCode::VERIFICATION_FAILED);

    vector<uint8_t> rootOfTrust;
    result = tee->getRootOfTrust(challenge, &rootOfTrust);
    ASSERT_TRUE(result.isOk());

    validateMacedRootOfTrust(rootOfTrust);

    vector<uint8_t> corruptedRootOfTrust = rootOfTrust;
    corruptedRootOfTrust[corruptedRootOfTrust.size() / 2]++;
    result = sb->sendRootOfTrust(corruptedRootOfTrust);
    ASSERT_FALSE(result.isOk());
    ASSERT_EQ(result.getExceptionCode(), EX_SERVICE_SPECIFIC);
    ASSERT_EQ(static_cast<ErrorCode>(result.getServiceSpecificError()),
              ErrorCode::VERIFICATION_FAILED);

    // Now try the correct RoT
    result = sb->sendRootOfTrust(rootOfTrust);
    ASSERT_TRUE(result.isOk());
}

}  // namespace aidl::android::hardware::security::keymint::test
