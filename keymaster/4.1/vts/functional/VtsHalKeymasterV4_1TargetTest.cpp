/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define LOG_TAG "keymaster_4_1_hidl_hal_test"

#include <android-base/logging.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.1/types.h>

#include <keymaster/keymaster_configuration.h>

#include <KeymasterHidlTest.h>
#include <KeymasterHidlTestUtils.h>

#include <keymasterV4_0/authorization_set.h>
#include <keymasterV4_1/attestation_record.h>

#include <openssl/asn1t.h>
#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/x509.h>

#include <VtsHalHidlTargetTestBase.h>
#include <VtsHalHidlTargetTestEnvBase.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace V4_1 {

namespace test {

using ::android::sp;
using ::std::string;

using V4_0::AuthorizationSet;
using V4_0::ErrorCode;
using V4_0::SecurityLevel;
using V4_0::test::HidlBuf;

class KeymasterHidlTest : public V4_0::test::KeymasterHidlTest {
   public:
    static void SetUpTestCase();

    static CertificationLevel CertLevel() { return certificationLevel_; }

   private:
    static CertificationLevel certificationLevel_;
};

CertificationLevel KeymasterHidlTest::certificationLevel_;

void KeymasterHidlTest::SetUpTestCase() {
    V4_0::test::KeymasterHidlTest::SetUpTestCase();
    ASSERT_TRUE(
        static_cast<IKeymasterDevice*>(&keymaster())
            ->getHardwareInfo_4_1([&](V4_0::SecurityLevel, const hidl_string&, const hidl_string&,
                                      CertificationLevel certificationLevel) {
                certificationLevel_ = certificationLevel;
            })
            .isOk());
}

TEST_F(KeymasterHidlTest, CertificationLevel) {
    ASSERT_EQ(ErrorCode::OK, GenerateKey(V4_0::AuthorizationSetBuilder()
                                             .Authorization(V4_0::TAG_NO_AUTH_REQUIRED)
                                             .EcdsaSigningKey(V4_0::EcCurve::P_256)
                                             .Digest(V4_0::Digest::SHA_2_256)
                                             .Authorization(V4_0::TAG_INCLUDE_UNIQUE_ID)));

    hidl_vec<hidl_vec<uint8_t>> cert_chain;
    ASSERT_EQ(ErrorCode::OK,
              AttestKey(V4_0::AuthorizationSetBuilder()
                            .Authorization(V4_0::TAG_ATTESTATION_CHALLENGE, HidlBuf("challenge"))
                            .Authorization(V4_0::TAG_ATTESTATION_APPLICATION_ID, HidlBuf("foo")),
                        &cert_chain));
    EXPECT_GE(cert_chain.size(), 2U);
    EXPECT_TRUE(V4_0::test::verify_chain(cert_chain));

    EXPECT_TRUE(V4_0::test::verify_attestation_record(
        "challenge", "foo", key_characteristics_.softwareEnforced,
        key_characteristics_.hardwareEnforced, SecLevel(), cert_chain[0]));

    X509_Ptr cert(V4_0::test::parse_cert_blob(cert_chain[0]));
    EXPECT_TRUE(!!cert.get());

    ASN1_OCTET_STRING* attest_rec = V4_0::test::get_attestation_record(cert.get());
    EXPECT_TRUE(!!attest_rec);

    AuthorizationSet att_sw_enforced;
    AuthorizationSet att_tee_enforced;
    uint32_t att_attestation_version;
    uint32_t att_keymaster_version;
    SecurityLevel att_attestation_security_level;
    SecurityLevel att_keymaster_security_level;
    HidlBuf att_challenge;
    HidlBuf att_unique_id;
    HidlBuf att_app_id;
    CertificationLevel att_certification_level;

    auto error = parse_attestation_record(attest_rec->data,                 //
                                          attest_rec->length,               //
                                          &att_attestation_version,         //
                                          &att_attestation_security_level,  //
                                          &att_keymaster_version,           //
                                          &att_keymaster_security_level,    //
                                          &att_challenge,                   //
                                          &att_sw_enforced,                 //
                                          &att_tee_enforced,                //
                                          &att_unique_id, &att_certification_level);
    EXPECT_EQ(ErrorCode::OK, error);

    EXPECT_EQ(CertLevel(), att_certification_level);
}

}  // namespace test
}  // namespace V4_1
}  // namespace keymaster
}  // namespace hardware
}  // namespace android
