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

#define LOG_TAG "keymint_1_bootloader_test"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <android-base/properties.h>
#include <android/binder_manager.h>
#include <remote_prov/remote_prov_utils.h>

#include "KeyMintAidlTestBase.h"

namespace aidl::android::hardware::security::keymint::test {

using ::android::getAidlHalInstanceNames;
using ::android::base::GetProperty;
using ::std::string;
using ::std::vector;

// Since this test needs to talk to KeyMint HAL, it can only run as root. Thus,
// bootloader can not be locked.
class BootloaderStateTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        keyMint_ = IKeyMintDevice::fromBinder(binder);
        ASSERT_TRUE(keyMint_) << "Failed to get KM device";
    }

    std::shared_ptr<IKeyMintDevice> keyMint_;
};

// Check that ro.boot.vbmeta.device_state is set unlocked.
TEST_P(BootloaderStateTest, PropsUnlocked) {
    auto deviceState = GetProperty("ro.boot.vbmeta.device_state", "");
    ASSERT_EQ(deviceState, "unlocked") << "This test runs as root. Bootloader must be unlocked.";
}

// Check that attested bootloader state is set to unlocked.
TEST_P(BootloaderStateTest, MatchesProp) {
    // Generate a key with attestation.
    AuthorizationSet keyDesc = AuthorizationSetBuilder()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .EcdsaSigningKey(EcCurve::P_256)
                                       .AttestationChallenge("foo")
                                       .AttestationApplicationId("bar")
                                       .Digest(Digest::NONE)
                                       .SetDefaultValidity();
    KeyCreationResult creationResult;
    auto kmStatus = keyMint_->generateKey(keyDesc.vector_data(), std::nullopt, &creationResult);
    ASSERT_TRUE(kmStatus.isOk());

    vector<Certificate> key_cert_chain = std::move(creationResult.certificateChain);

    // Parse attested AVB values.
    vector<uint8_t> key;
    VerifiedBoot attestedVbState;
    bool attestedBootloaderState;
    vector<uint8_t> attestedVbmetaDigest;
    parse_root_of_trust(key_cert_chain[0].encodedCertificate, &key, &attestedVbState,
                        &attestedBootloaderState, &attestedVbmetaDigest);

    ASSERT_FALSE(attestedBootloaderState) << "This test runs as root. Bootloader must be unlocked.";
}

INSTANTIATE_TEST_SUITE_P(PerInstance, BootloaderStateTest,
                         testing::ValuesIn(getAidlHalInstanceNames(IKeyMintDevice::descriptor)),
                         ::android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::security::keymint::test
