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

#include "Keymaster4_1HidlTest.h"

#include <cutils/log.h>
#include <cutils/properties.h>
#include <keymasterV4_0/key_param_output.h>
#include <keymasterV4_1/authorization_set.h>
#include <signal.h>
#include <functional>
#include <iostream>
#include <string>

namespace android::hardware::keymaster::V4_1::test {

using std::string;

using OaepMGFDigestTest = Keymaster4_1HidlTest;
/*
 * OaepMGFDigestTest.RsaOaepMGFSuccess
 *
 * Verifies that RSA-OAEP encryption operations work, with all MGF digests.
 */
TEST_P(OaepMGFDigestTest, RsaOaepMGFSuccess) {
    auto mgf_digests = ValidDigests(false /* withNone */, false /* withMD5 */);

    size_t key_size = 2048;  // Need largish key for SHA-512 test.
    AuthorizationSet4_1Builder authSet = AuthorizationSet4_1Builder();
    ASSERT_EQ(V4_0::ErrorCode::OK, GenerateKey(authSet.OaepMGFDigest(mgf_digests)
                                                       .Digest(Digest::SHA_2_256)
                                                       .RsaEncryptionKey(key_size, 65537)
                                                       .Padding(PaddingMode::RSA_OAEP)));

    string message = "Hello";
    for (auto digest : mgf_digests) {
        auto params = AuthorizationSet4_1Builder()
                              .Authorization(TAG_RSA_OAEP_MGF_DIGEST, digest)
                              .Digest(Digest::SHA_2_256)
                              .Padding(PaddingMode::RSA_OAEP);
        string ciphertext1 = EncryptMessage(message, params);
        if (HasNonfatalFailure()) std::cout << "-->" << digest << std::endl;
        EXPECT_EQ(key_size / 8, ciphertext1.size());

        string ciphertext2 = EncryptMessage(message, params);
        EXPECT_EQ(key_size / 8, ciphertext2.size());

        // OAEP randomizes padding so every result should be different (with astronomically high
        // probability).
        EXPECT_NE(ciphertext1, ciphertext2);
        string plaintext1 = DecryptMessage(ciphertext1, params);
        EXPECT_EQ(message, plaintext1) << "RSA-OAEP failed with MGF digest " << digest;
        string plaintext2 = DecryptMessage(ciphertext2, params);
        EXPECT_EQ(message, plaintext2) << "RSA-OAEP failed with MGF digest " << digest;

        // Decrypting corrupted ciphertext should fail.
        size_t offset_to_corrupt = random() % ciphertext1.size();
        char corrupt_byte;
        do {
            corrupt_byte = static_cast<char>(random() % 256);
        } while (corrupt_byte == ciphertext1[offset_to_corrupt]);
        ciphertext1[offset_to_corrupt] = corrupt_byte;

        EXPECT_EQ(V4_0::ErrorCode::OK, Begin(KeyPurpose::DECRYPT, params));
        string result;
        EXPECT_EQ(V4_0::ErrorCode::UNKNOWN_ERROR, Finish(ciphertext1, &result));
        EXPECT_EQ(0U, result.size());
    }
}

/*
 * OaepMGFDigestTest.RsaOaepMGFIncompatibleDigest
 *
 * Verifies that RSA-OAEP encryption operations fails with error if mgf digest specified in
 * key characteristics does not match with that in begin operation.
 */
TEST_P(OaepMGFDigestTest, RsaOaepMGFIncompatibleDigest) {
    AuthorizationSet4_1Builder authSet = AuthorizationSet4_1Builder();
    ASSERT_EQ(V4_0::ErrorCode::OK,
              GenerateKey(authSet.Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_256)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaEncryptionKey(2048, 65537)
                                  .Padding(PaddingMode::RSA_OAEP)
                                  .Digest(Digest::SHA_2_256)));
    auto params = AuthorizationSet4_1Builder()
                          .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_224)
                          .Padding(PaddingMode::RSA_OAEP)
                          .Digest(Digest::SHA_2_256);
    EXPECT_EQ(ErrorCode::INCOMPATIBLE_MGF_DIGEST, BeginMessage(KeyPurpose::ENCRYPT, params));
}

/*
 * OaepMGFDigestTest.RsaOaepMGFIncompatibleDigest
 *
 * Verifies that RSA-OAEP encryption operations fails with error if no mgf digest is specified in
 * in the begin operation.
 */
TEST_P(OaepMGFDigestTest, RsaOaepMGFUnsupportedDigest) {
    AuthorizationSet4_1Builder authSet = AuthorizationSet4_1Builder();
    ASSERT_EQ(V4_0::ErrorCode::OK,
              GenerateKey(authSet.Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::SHA_2_256)
                                  .Authorization(TAG_NO_AUTH_REQUIRED)
                                  .RsaEncryptionKey(2048, 65537)
                                  .Padding(PaddingMode::RSA_OAEP)
                                  .Digest(Digest::SHA_2_256)));
    auto params = AuthorizationSet4_1Builder()
                          .Authorization(TAG_RSA_OAEP_MGF_DIGEST, Digest::NONE)
                          .Padding(PaddingMode::RSA_OAEP)
                          .Digest(Digest::SHA_2_256);
    EXPECT_EQ(ErrorCode::UNSUPPORTED_MGF_DIGEST, BeginMessage(KeyPurpose::ENCRYPT, params));
}

INSTANTIATE_KEYMASTER_4_1_HIDL_TEST(OaepMGFDigestTest);

}  // namespace android::hardware::keymaster::V4_1::test
