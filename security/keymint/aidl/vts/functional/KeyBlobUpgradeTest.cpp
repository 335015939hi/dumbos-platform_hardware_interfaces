/*
 * Copyright (C) 2022 The Android Open Source Project
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

// The tests in this file are intended to be run manually, to allow testing of whether
// keyblob upgrade works correctly.  The manual procedure is roughly:
//
// - Run all these tests with the `--keyblob_dir <dir>` command-line argument so that keyblobs are
//   saved to a directory on the device.
//   - All tests should pass, and the `UpgradeKeyBlobs` test should indicate that no keyblob
//     upgrades were needed.
// - Copy the generated keyblobs off the device into a safe place.
// - Upgrade the device to a new version.
// - Push the saved keyblobs back onto the upgraded device.
// - (**Don't** run the `CreateKeyBlobs` test, as that might replace the saved keyblobs with
//   freshly generated ones)
// - Run the `UpgradeKeyBlobs` test to replace the keyblobs with upgraded keyblobs.
// - Run the `UseKeyBlobs` test to confirm that the upgraded keyblobs still work.

#define LOG_TAG "keymint_1_test"
#include <cutils/log.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include "KeyMintAidlTestBase.h"

using aidl::android::hardware::security::keymint::AuthorizationSet;
using aidl::android::hardware::security::keymint::KeyCharacteristics;

namespace aidl::android::hardware::security::keymint::test {

namespace {

std::vector<std::string> keyblob_names = {"aes-key",         "aes-key-rr",        "des-key",
                                          "hmac-key",        "rsa-key",           "p256-key",
                                          "ed25519-key",     "x25519-key",        "rsa-attest-key",
                                          "p256-attest-key", "ed25519-attest-key"};

std::string keyblob_subdir(const std::string& keyblob_dir, const std::string& full_name,
                           bool create) {
    if (keyblob_dir.empty()) {
        return "";
    }

    // Use a subdirectory for the specific instance, so two different KeyMint instances won't
    // clash with each other.
    size_t found = full_name.find_last_of("/");
    std::string subdir = keyblob_dir + "/" + full_name.substr(found + 1);

    if (create) {
        mkdir(keyblob_dir.c_str(), 0777);
        mkdir(subdir.c_str(), 0777);
    }
    return subdir;
}

void save_keyblob(const std::string& subdir, const std::string& name,
                  const vector<uint8_t>& keyblob,
                  const std::vector<KeyCharacteristics> key_characteristics) {
    // Write the keyblob out to a file.
    std::string blobname(subdir + "/" + name + ".keyblob");
    std::ofstream blobfile(blobname, std::ios::out | std::ios::trunc | std::ios::binary);
    blobfile.write(reinterpret_cast<const char*>(keyblob.data()), keyblob.size());
    blobfile.close();

    // Dump the characteristics too.
    std::string charsname(subdir + "/" + name + ".chars");
    std::ofstream charsfile(charsname, std::ios::out | std::ios::trunc);
    charsfile << "{\n";
    for (const auto& characteristic : key_characteristics) {
        charsfile << "  " << characteristic.toString() << "\n";
    }
    charsfile << "}\n";
    charsfile.close();

    // Also write out a hexdump of the keyblob for convenience.
    std::string hexname(subdir + "/" + name + ".hex");
    std::ofstream hexfile(hexname, std::ios::out | std::ios::trunc);
    hexfile << bin2hex(keyblob) << "\n";
    hexfile.close();
}

std::vector<uint8_t> load_keyblob(const std::string& subdir, const std::string& name) {
    std::string blobname(subdir + "/" + name + ".keyblob");
    std::ifstream blobfile(blobname, std::ios::in | std::ios::binary);

    std::vector<uint8_t> data((std::istreambuf_iterator<char>(blobfile)),
                              std::istreambuf_iterator<char>());
    return data;
}

bool requires_rr(const std::string& name) {
    return name.find("-rr") != std::string::npos;
}

}  // namespace

using KeyBlobUpgradeTest = KeyMintAidlTestBase;

// To save off keyblobs before upgrade, use:
//
//    VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.CreateKeyBlobs*" \
//                             --keyblob_dir /data/local/tmp/keymint-blobs
//
// Then copy the contents of the /data/local/tmp/keymint-blobs/ directory somewhere safe:
//
//    adb pull /data/local/tmp/keymint-blobs/
TEST_P(KeyBlobUpgradeTest, CreateKeyBlobs) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ true);

    std::map<const std::string, AuthorizationSetBuilder> keys_info = {
            {"aes-key", AuthorizationSetBuilder()
                                .AesEncryptionKey(256)
                                .BlockMode(BlockMode::ECB)
                                .Padding(PaddingMode::PKCS7)
                                .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"aes-key-rr", AuthorizationSetBuilder()
                                   .AesEncryptionKey(256)
                                   .BlockMode(BlockMode::ECB)
                                   .Padding(PaddingMode::PKCS7)
                                   .Authorization(TAG_ROLLBACK_RESISTANCE)
                                   .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"des-key", AuthorizationSetBuilder()
                                .TripleDesEncryptionKey(168)
                                .BlockMode(BlockMode::ECB)
                                .Padding(PaddingMode::PKCS7)
                                .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"hmac-key", AuthorizationSetBuilder()
                                 .HmacKey(128)
                                 .Digest(Digest::SHA1)
                                 .Authorization(TAG_MIN_MAC_LENGTH, 128)
                                 .Authorization(TAG_NO_AUTH_REQUIRED)},
            {"rsa-key", AuthorizationSetBuilder()
                                .RsaEncryptionKey(2048, 65537)
                                .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                                .Digest(Digest::NONE)
                                .Digest(Digest::SHA1)
                                .Padding(PaddingMode::NONE)
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .SetDefaultValidity()},
            {
                    "p256-key",
                    AuthorizationSetBuilder()
                            .EcdsaSigningKey(EcCurve::P_256)
                            .Authorization(TAG_PURPOSE, KeyPurpose::AGREE_KEY)
                            .Digest(Digest::NONE)
                            .Digest(Digest::SHA1)
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {
                    "ed25519-key",
                    AuthorizationSetBuilder()
                            .EcdsaSigningKey(EcCurve::CURVE_25519)
                            .Digest(Digest::NONE)
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {"x25519-key", AuthorizationSetBuilder()
                                   .Authorization(TAG_EC_CURVE, EcCurve::CURVE_25519)
                                   .Authorization(TAG_PURPOSE, KeyPurpose::AGREE_KEY)
                                   .Authorization(TAG_ALGORITHM, Algorithm::EC)
                                   .Authorization(TAG_NO_AUTH_REQUIRED)
                                   .SetDefaultValidity()},
            {"rsa-attest-key", AuthorizationSetBuilder()
                                       .RsaKey(2048, 65537)
                                       .AttestKey()
                                       .Authorization(TAG_NO_AUTH_REQUIRED)
                                       .SetDefaultValidity()},
            {
                    "p256-attest-key",
                    AuthorizationSetBuilder()
                            .EcdsaKey(EcCurve::P_256)
                            .AttestKey()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            },
            {
                    "ed25519-attest-key",
                    AuthorizationSetBuilder()
                            .EcdsaKey(EcCurve::CURVE_25519)
                            .AttestKey()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .SetDefaultValidity(),
            }};

    for (std::string name : keyblob_names) {
        auto entry = keys_info.find(name);
        ASSERT_NE(entry, keys_info.end()) << "no builder for " << name;
        auto builder = entry->second;
        for (bool with_hidden : {false, true}) {
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            SCOPED_TRACE(testing::Message() << name);

            vector<uint8_t> keyblob;
            vector<KeyCharacteristics> key_characteristics;
            auto result = GenerateKey(builder, &keyblob, &key_characteristics);

            if (requires_rr(name) && result == ErrorCode::ROLLBACK_RESISTANCE_UNAVAILABLE) {
                // Rollback resistance support is optional.
                std::cout << "Skipping '" << name << "' key as rollback resistance unavailable\n";
                continue;
            }
            ASSERT_EQ(ErrorCode::OK, result) << " failed for " << name;

            if (!subdir.empty()) {
                save_keyblob(subdir, name, keyblob, key_characteristics);
            }
        }
    }

    if (!subdir.empty()) {
        std::cerr << "Save generated keyblobs with:\n\n    adb pull " << keyblob_dir << "\n\n";
    }
}

// To run this test:
//
// - save off some keyblobs before upgrade as per the CreateKeyBlobs test above.
// - upgrade the device
// - put the saved keyblobs back onto the upgraded device:
//
//     adb push keymint-blobs /data/local/tmp/keymint-blobs
//
// - run the test with:
//
//     VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.UpgradeKeyBlobs*" \
//                              --keyblob_dir /data/local/tmp/keymint-blobs
//
// - this replaces the keyblob contents in that directory; if needed, save the upgraded keyblobs
// with:
//    adb pull /data/local/tmp/keymint-blobs/
TEST_P(KeyBlobUpgradeTest, UpgradeKeyBlobs) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ false);
    if (subdir.empty()) {
        GTEST_SKIP() << "No keyblob directory provided";
    }

    for (std::string name : keyblob_names) {
        for (bool with_hidden : {false, true}) {
            std::string app_id;
            std::string app_data;
            auto builder = AuthorizationSetBuilder();
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                app_id = "appid";
                app_data = "appdata";
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            SCOPED_TRACE(testing::Message() << name);

            // Load the old format keyblob.
            std::vector<uint8_t> keyblob = load_keyblob(subdir, name);
            if (keyblob.empty()) {
                if (requires_rr(name)) {
                    std::cout << "Skipping missing keyblob file  '" << name
                              << "', assuming rollback resistance unavailable\n";
                } else {
                    FAIL() << "Missing keyblob file '" << name << "'";
                }
                continue;
            }

            // An upgrade will either produce a new keyblob or no data (if upgrade isn't needed).
            std::vector<uint8_t> upgraded_keyblob;
            Status result = keymint_->upgradeKey(keyblob, builder.vector_data(), &upgraded_keyblob);
            ASSERT_EQ(ErrorCode::OK, GetReturnErrorCode(result));

            if (upgraded_keyblob.empty()) {
                std::cerr << "Keyblob '" << name << "' did not require upgrade\n";
            } else {
                std::vector<uint8_t> app_id_v(app_id.begin(), app_id.end());
                std::vector<uint8_t> app_data_v(app_id.begin(), app_id.end());
                std::vector<KeyCharacteristics> key_characteristics;
                result = keymint_->getKeyCharacteristics(upgraded_keyblob, app_id_v, app_data_v,
                                                         &key_characteristics);
                ASSERT_EQ(ErrorCode::OK, GetReturnErrorCode(result));

                save_keyblob(subdir, name, upgraded_keyblob, key_characteristics);
            }
        }
    }
}

// To run this test:
//
// - save off some keyblobs before upgrade as per the CreateKeyBlobs test above
// - if needed, upgrade the saved keyblobs as per the UpgradeKeyBlobs test above
// - run the test with:
//
//     VtsAidlKeyMintTargetTest --gtest_filter="*KeyBlobUpgradeTest.UseKeyBlobs*" \
//                              --keyblob_dir /data/local/tmp/keymint-blobs
TEST_P(KeyBlobUpgradeTest, UseKeyBlobs) {
    std::string subdir = keyblob_subdir(keyblob_dir, GetParam(), /* create? */ false);
    if (subdir.empty()) {
        GTEST_SKIP() << "No keyblob directory provided";
    }

    for (std::string name : keyblob_names) {
        for (bool with_hidden : {false, true}) {
            std::string app_id;
            std::string app_data;
            auto builder = AuthorizationSetBuilder();
            if (with_hidden) {
                // Build a variant keyblob that requires app_id/app_data
                app_id = "appid";
                app_data = "appdata";
                builder.Authorization(TAG_APPLICATION_ID, "appid")
                        .Authorization(TAG_APPLICATION_DATA, "appdata");
                name += "-hidden";
            }
            SCOPED_TRACE(testing::Message() << name);
            std::vector<uint8_t> keyblob = load_keyblob(subdir, name);
            if (keyblob.empty()) {
                if (requires_rr(name)) {
                    std::cout << "Skipping missing keyblob file  '" << name
                              << "', assuming rollback resistance unavailable\n";
                } else {
                    FAIL() << "Missing keyblob file '" << name << "'";
                }
                continue;
            }

            // Perform an algorithm-specific operation with the keyblob.
            string message = "Hello World!";
            AuthorizationSet out_params;
            if (name.find("aes-key") != std::string::npos) {
                builder.BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
                string ciphertext = EncryptMessage(keyblob, message, builder, &out_params);
                string plaintext = DecryptMessage(keyblob, ciphertext, builder);
                EXPECT_EQ(message, plaintext);
            } else if (name.find("des-key") != std::string::npos) {
                builder.BlockMode(BlockMode::ECB).Padding(PaddingMode::PKCS7);
                string ciphertext = EncryptMessage(keyblob, message, builder, &out_params);
                string plaintext = DecryptMessage(keyblob, ciphertext, builder);
                EXPECT_EQ(message, plaintext);
            } else if (name.find("hmac-key") != std::string::npos) {
                builder.Digest(Digest::SHA1).Authorization(TAG_MAC_LENGTH, 128);
                string tag = SignMessage(keyblob, message, builder);
                VerifyMessage(keyblob, message, tag, builder);
            } else if (name.find("rsa-key") != std::string::npos) {
            } else if (name.find("p256-key") != std::string::npos) {
            } else if (name.find("ed25519-key") != std::string::npos) {
            } else if (name.find("x25519-key") != std::string::npos) {
            } else if (name.find("rsa-attest-key") != std::string::npos) {
            } else if (name.find("p256-attest-key") != std::string::npos) {
            } else if (name.find("ed25519-attest-key") != std::string::npos) {
            } else {
                FAIL() << "Unexpected name: " << name;
            }
        }
    }
}

INSTANTIATE_KEYMINT_AIDL_TEST(KeyBlobUpgradeTest);

}  // namespace aidl::android::hardware::security::keymint::test
