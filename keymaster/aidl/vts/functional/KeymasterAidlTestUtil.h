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

#ifndef VTS_KEYMASTER_AIDL_TEST_UTILS_H
#define VTS_KEYMASTER_AIDL_TEST_UTILS_H

#pragma once

#include <andl/android/hardware/keymaster/IKeymasterDevice.h>
//#include <android/hardware/keymaster/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

//#include <keymasterV4_0/authorization_set.h>

namespace android {
namespace hardware {
namespace keymaster {
namespace test_utils {

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set);

namespace test {

using ::android::sp;
using hidl::base::V1_0::DebugInfo;
using ::std::string;
using std::vector;

constexpr uint64_t kOpHandleSentinel = 0xFFFFFFFFFFFFFFFF;

class KeymasterAidlTestUtil : public ::testing::TestWithParam<string> {
  public:
    void SetUp() override;
    void TearDown() override {
        if (key_blob_.size()) {
            CheckedDeleteKey();
        }
        AbortIfNeeded();
    }

    void InitializeKeymaster(sp<IKeymasterDevice> keymaster);
    IKeymasterDevice& keymaster() { return *keymaster_; }
    uint32_t os_version() { return os_version_; }
    uint32_t os_patch_level() { return os_patch_level_; }

    Status::Exception GenerateKey(const AuthorizationSet& key_desc, vector<uint8_t>* key_blob,
                                  KeyCharacteristics* key_characteristics,
                                  vector<Certificate>* certChain);
    Status::Exception GenerateKey(const AuthorizationSet& key_desc);

    Status::Exception ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                const string& key_material, vector<uint8_t>* key_blob,
                                KeyCharacteristics* key_characteristics);
    Status::Exception ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                const string& key_material);

    Status::Exception ImportWrappedKey(string wrapped_key, string wrapping_key,
                                       const AuthorizationSet& wrapping_key_desc,
                                       string masking_key,
                                       const AuthorizationSet& unwrapping_params);

    Status::Exception DeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob = false);
    Status::Exception DeleteKey(bool keep_key_blob = false);

    Status::Exception DeleteAllKeys();

    void CheckedDeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob = false);
    void CheckedDeleteKey();

    void CheckGetCharacteristics(const vector<uint8_t>& key_blob, const vector<uint8_t>& client_id,
                                 const vector<uint8_t>& app_data,
                                 KeyCharacteristics* key_characteristics);
    Status::Exception GetCharacteristics(const vector<uint8_t>& key_blob,
                                         const vector<uint8_t>& client_id,
                                         const vector<uint8_t>& app_data,
                                         KeyCharacteristics* key_characteristics);
    Status::Exception GetCharacteristics(const vector<uint8_t>& key_blob,
                                         KeyCharacteristics* key_characteristics);

    Status::Exception GetDebugInfo(DebugInfo* debug_info);

    Status::Exception Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                            const AuthorizationSet& in_params, AuthorizationSet* out_params);
    Status::Exception Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                            AuthorizationSet* out_params);
    Status::Exception Begin(KeyPurpose purpose, const AuthorizationSet& in_params);

    Status::Exception Update(const AuthorizationSet& in_params, const string& input,
                             AuthorizationSet* out_params, string* output, size_t* input_consumed);
    Status::Exception Update(const string& input, string* out, size_t* input_consumed);

    Status::Exception Finish(const AuthorizationSet& in_params, const string& input,
                             const string& signature, AuthorizationSet* out_params, string* output);
    Status::Exception Finish(const string& message, string* output);
    Status::Exception Finish(const string& message, const string& signature, string* output);
    Status::Exception Finish(string* output) { return Finish(string(), output); }

    Status::Exception Abort();

    void AbortIfNeeded();

    string ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                          const string& message, const AuthorizationSet& in_params,
                          AuthorizationSet* out_params);

    string SignMessage(const vector<uint8_t>& key_blob, const string& message,
                       const AuthorizationSet& params);
    string SignMessage(const string& message, const AuthorizationSet& params);

    string MacMessage(const string& message, Digest digest, size_t mac_length);

    void CheckHmacTestVector(const string& key, const string& message, Digest digest,
                             const string& expected_mac);

    void CheckAesCtrTestVector(const string& key, const string& nonce, const string& message,
                               const string& expected_ciphertext);

    void CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                  PaddingMode padding_mode, const string& key, const string& iv,
                                  const string& input, const string& expected_output);

    void VerifyMessage(const vector<uint8_t>& key_blob, const string& message,
                       const string& signature, const AuthorizationSet& params);
    void VerifyMessage(const string& message, const string& signature,
                       const AuthorizationSet& params);

    string EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                          const AuthorizationSet& in_params, AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params,
                          AuthorizationSet* out_params);
    string EncryptMessage(const string& message, const AuthorizationSet& params);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          vector<uint8_t>* iv_out);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          const vector<uint8_t>& iv_in);
    string EncryptMessage(const string& message, BlockMode block_mode, PaddingMode padding,
                          uint8_t mac_length_bits, const vector<uint8_t>& iv_in);

    string DecryptMessage(const vector<uint8_t>& key_blob, const string& ciphertext,
                          const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, const AuthorizationSet& params);
    string DecryptMessage(const string& ciphertext, BlockMode block_mode, PaddingMode padding_mode,
                          const vector<uint8_t>& iv);

    std::pair<ErrorCode, vector<uint8_t>> UpgradeKey(const vector<uint8_t>& key_blob);

    bool IsSecure() { return securityLevel_ != SecurityLevel::SOFTWARE; }
    SecurityLevel SecLevel() { return securityLevel_; }

    vector<uint32_t> ValidKeySizes(Algorithm algorithm);
    vector<uint32_t> InvalidKeySizes(Algorithm algorithm);

    vector<EcCurve> ValidCurves();
    vector<EcCurve> InvalidCurves();

    vector<Digest> ValidDigests(bool withNone, bool withMD5);
    vector<Digest> InvalidDigests();

    vector<Certificate> certChain_;
    vector<uint8_t> key_blob_;
    KeyCharacteristics key_characteristics_;

    static vector<string> build_params() {
        auto params = android::hardware::getAllHalInstanceNames(IKeymasterDevice::descriptor);
        return params;
    }

  private:
    sp<IKeymasterDevice> keymaster_;
    uint32_t os_version_;
    uint32_t os_patch_level_;

    SecurityLevel securityLevel_;
    hidl_string name_;
    hidl_string author_;
    shared_ptr<IKeymasterOperation> op_;
    long challenge_;
};

#define INSTANTIATE_KEYMASTER_AIDL_TEST(name)                                          \
    INSTANTIATE_TEST_SUITE_P(PerInstance, name,                                        \
                             testing::ValuesIn(KeymasterAidlTestUtil::build_params()), \
                             android::hardware::PrintInstanceNameToString)

}  // namespace test
}  // namespace test_utils
}  // namespace keymaster
}  // namespace hardware

#endif  // VTS_KEYMASTER_AIDL_TEST_UTILS_H
