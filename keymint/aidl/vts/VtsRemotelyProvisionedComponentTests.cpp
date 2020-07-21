/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "VtsRemotelyProvisionableComponentTests"

#include <aidl/android/hardware/keymint/IRemotelyProvisionedComponent.h>

#include <RemotelyProvisionedComponent.h>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>
#include <binder/IServiceManager.h>
#include <cppbor_parse.h>
#include <cppcose/cppcose.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <remote_prov/remote_prov_utils.h>

namespace aidl::android::hardware::keymint::test {

using bytevec = std::vector<uint8_t>;
using testing::MatchesRegex;
using namespace remote_prov;

class VtsRemotelyProvisionedComponentTests : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        // provisionable_ = android::waitForDeclaredService<IRemotelyProvisionedComponent>(
        //         String16(GetParam().c_str()));
        // ASSERT_NE(provisionable_, nullptr);
        provisionable_ = ndk::SharedRefBase::make<RemotelyProvisionedComponent>();
    }

  protected:
    std::shared_ptr<IRemotelyProvisionedComponent> provisionable_;
};

using GenerateKeyTests = VtsRemotelyProvisionedComponentTests;

TEST_F(GenerateKeyTests, generateEdd25519Key) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    auto status = provisionable_->generateEd25519KeyPair(false /* test mode */, &macedPubKey,
                                                         &privateKeyBlob);
    ASSERT_TRUE(status.isOk());
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.key);
    ASSERT_TRUE(coseMac0) << "COSE Mac0 parse failed " << mac0ParseErr;

    ASSERT_NE(coseMac0->asArray(), nullptr);
    ASSERT_EQ(coseMac0->asArray()->size(), 4U);

    auto protParms = coseMac0->asArray()->get(0)->asBstr();
    ASSERT_NE(protParms, nullptr);
    ASSERT_EQ(cppbor::prettyPrint(protParms->value()), "{\n  1 : 5,\n}");

    auto unprotParms = coseMac0->asArray()->get(1)->asBstr();
    ASSERT_NE(unprotParms, nullptr);
    ASSERT_EQ(unprotParms->value().size(), 0);

    auto payload = coseMac0->asArray()->get(2)->asBstr();
    ASSERT_NE(payload, nullptr);
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(payload->value());
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;
    EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                MatchesRegex("{\n"
                             "  1 : 1,\n"
                             "  3 : -8,\n"
                             "  -1 : 6,\n"
                             // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a sequence of
                             // 32 hexadecimal bytes, enclosed in braces and separated by commas.
                             // In this case, some Ed25519 public key.
                             "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"
                             "}"));

    auto coseMac0Tag = coseMac0->asArray()->get(3)->asBstr();
    ASSERT_TRUE(coseMac0);
    auto extractedTag = coseMac0Tag->value();
    EXPECT_EQ(extractedTag.size(), 32U);

    // Compare with tag generated with kTestMacKey.  Shouldn't match.
    auto testTag = cppcose::generateCoseMac0Mac(remote_prov::kTestMacKey, {} /* external_aad */,
                                                payload->value());
    ASSERT_TRUE(testTag) << "Tag calculation failed: " << testTag.message();

    EXPECT_NE(*testTag, extractedTag);
}

TEST_F(GenerateKeyTests, generateEdd25519TestKey) {
    MacedPublicKey macedPubKey;
    bytevec privateKeyBlob;
    auto status = provisionable_->generateEd25519KeyPair(true /* test mode */, &macedPubKey,
                                                         &privateKeyBlob);

    ASSERT_TRUE(status.isOk());
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.key);
    ASSERT_TRUE(coseMac0) << "COSE Mac0 parse failed " << mac0ParseErr;

    ASSERT_NE(coseMac0->asArray(), nullptr);
    ASSERT_EQ(coseMac0->asArray()->size(), 4U);

    auto protParms = coseMac0->asArray()->get(0)->asBstr();
    ASSERT_NE(protParms, nullptr);
    ASSERT_EQ(cppbor::prettyPrint(protParms->value()), "{\n  1 : 5,\n}");

    auto unprotParms = coseMac0->asArray()->get(1)->asBstr();
    ASSERT_NE(unprotParms, nullptr);
    ASSERT_EQ(unprotParms->value().size(), 0);

    auto payload = coseMac0->asArray()->get(2)->asBstr();
    ASSERT_NE(payload, nullptr);
    auto [parsedPayload, __, payloadParseErr] = cppbor::parse(payload->value());
    ASSERT_TRUE(parsedPayload) << "Key parse failed: " << payloadParseErr;
    EXPECT_THAT(cppbor::prettyPrint(parsedPayload.get()),
                MatchesRegex("{\n"
                             "  1 : 1,\n"
                             "  3 : -8,\n"
                             "  -1 : 6,\n"
                             // The regex {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}} matches a sequence of
                             // 32 hexadecimal bytes, enclosed in braces and separated by commas.
                             // In this case, some Ed25519 public key.
                             "  -2 : {(0x[0-9a-f]{2}, ){31}0x[0-9a-f]{2}},\n"
                             "  -70000 : null,\n"
                             "}"));

    auto coseMac0Tag = coseMac0->asArray()->get(3)->asBstr();
    ASSERT_TRUE(coseMac0);
    auto extractedTag = coseMac0Tag->value();
    EXPECT_EQ(extractedTag.size(), 32U);

    auto testTag = cppcose::generateCoseMac0Mac(remote_prov::kTestMacKey, {} /* external_aad */,
                                                payload->value());
    ASSERT_TRUE(testTag) << testTag.message();

    EXPECT_EQ(*testTag, extractedTag);
}

class CertificateRequestTest : public VtsRemotelyProvisionedComponentTests {
  protected:
};

bytevec string_to_bytevec(const char* s) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(s);
    return bytevec(p, p + strlen(s));
}

TEST_F(CertificateRequestTest, Success_testMode) {
    auto eekId = string_to_bytevec("eekid");
    auto eekChain = generateEekChain(3, string_to_bytevec("eekid"));
    ASSERT_TRUE(eekChain) << eekChain.message();

    bool testMode = true;
    bytevec keysToSignMac;
    bytevec protectedData;
    auto challenge = string_to_bytevec("challenge");
    auto status = provisionable_->generateCertificateRequest(testMode, {} /* keysToSign */,
                                                             eekChain->chain, challenge,
                                                             &keysToSignMac, &protectedData);
    ASSERT_TRUE(status.isOk()) << status.getMessage();

    auto [parsedProtectedData, _, protDataErrMsg] = cppbor::parse(protectedData);
    ASSERT_TRUE(parsedProtectedData) << protDataErrMsg;
    ASSERT_TRUE(parsedProtectedData->asArray());
    ASSERT_EQ(parsedProtectedData->asArray()->size(), kCoseEncryptEntryCount);

    auto senderPubkey = getSenderPubKeyFromCoseEncrypt(parsedProtectedData);
    ASSERT_TRUE(senderPubkey);
    EXPECT_EQ(senderPubkey->second, eekId);

    auto sessionKey = x25519_HKDF_DeriveKey(eekChain->last_pubkey, eekChain->last_privkey,
                                            senderPubkey->first, false /* senderIsA */);
    ASSERT_TRUE(sessionKey) << sessionKey.message();

    auto protectedDataPayload =
            decryptCoseEncrypt(*sessionKey, parsedProtectedData.get(), bytevec{} /* aad */);
    ASSERT_TRUE(protectedDataPayload) << protectedDataPayload.message();

    auto [parsedPayload, __, payloadErrMsg] = cppbor::parse(*protectedDataPayload);
    ASSERT_TRUE(parsedPayload) << "Failed to parse payload: " << payloadErrMsg;
    ASSERT_TRUE(parsedPayload->asArray());
    EXPECT_EQ(parsedPayload->asArray()->size(), 2U);

    auto& signedMac = parsedPayload->asArray()->get(0);
    auto& bcc = parsedPayload->asArray()->get(1);
    ASSERT_TRUE(signedMac && signedMac->asArray());
    ASSERT_TRUE(bcc);

    auto bccContents = validateBcc(bcc->asArray());
    ASSERT_TRUE(bccContents) << "\n" << prettyPrint(bcc.get());
    ASSERT_GT(bccContents->size(), 0U);

    auto& signingKey = bccContents->back().pubKey;
    auto macKey = verifyAndParseCoseSign1(testMode, signedMac->asArray(), signingKey,
                                          cppbor::Array()          // DeviceInfo
                                                  .add(challenge)  //
                                                  .add(cppbor::Array())
                                                  .encode());
    ASSERT_TRUE(macKey);
}

// INSTANTIATE_TEST_SUITE_P(RemotelyProvisionedComponent, VtsRemotelyProvisionedComponentTests,
//                          testing::ValuesIn(android::getAidlHalInstanceNames(
//                                  IRemotelyProvisionedComponent::descriptor)),
//                          android::PrintInstanceNameToString);

}  // namespace aidl::android::hardware::keymint::test
