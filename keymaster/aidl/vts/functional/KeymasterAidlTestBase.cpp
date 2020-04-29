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

#include "KeymasterAidlTestBase.h"

#include <chrono>
#include <vector>

#include <android-base/logging.h>

#include <keymasterV5_0/key_param_output.h>
#include <keymasterV5_0/keymaster_utils.h>

namespace android {
namespace hardware {
namespace keymaster {

using namespace std::literals::chrono_literals;
using std::endl;

using namespace android::hardware::keymaster;
using ::android::hardware::keymaster::AuthorizationSet;

::std::ostream& operator<<(::std::ostream& os, const AuthorizationSet& set) {
    if (set.size() == 0)
        os << "(Empty)" << ::std::endl;
    else {
        os << "\n";
        for (size_t i = 0; i < set.size(); ++i) os << set[i] << ::std::endl;
    }
    return os;
}

void KeymasterAidlTestBase::InitializeKeymaster(sp<IKeymasterDevice> keymaster) {
    ASSERT_NE(keymaster, nullptr);
    keymaster_ = keymaster;

    KeymasterHardwareInfo info;
    ASSERT_TRUE(keymaster_->getHardwareInfo(&info).isOk());

    securityLevel_ = info.securityLevel;
    name_.assign(info.keymasterName.begin(), info.keymasterName.end());
    author_.assign(info.keymasterAuthorName.begin(), info.keymasterAuthorName.end());

    os_version_ = getOsVersion();
    os_patch_level_ = getOsPatchlevel();
}

void KeymasterAidlTestBase::SetUp() {
    InitializeKeymaster(
            android::waitForDeclaredService<IKeymasterDevice>(String16(GetParam().c_str())));
}

ErrorCode KeymasterAidlTestBase::GenerateKey(const AuthorizationSet& key_desc,
                                             vector<uint8_t>* keyBlob,
                                             KeyCharacteristics* keyChar) {
    EXPECT_NE(keyBlob, nullptr) << "Key blob pointer must not be null.  Test bug";
    EXPECT_EQ(0U, keyBlob->size()) << "Key blob not empty before generating key.  Test bug.";
    EXPECT_NE(keyChar, nullptr)
            << "Previous characteristics not deleted before generating key.  Test bug.";

    // Aidl does not clear these output parameters if the function returns
    // error.  This is different from hal where output parameter is always
    // cleared due to hal returning void.  So now we need to do our own clearing
    // of the output variables prior to calling keymaster aidl libraries.
    keyBlob->clear();
    keyChar->softwareEnforced.clear();
    keyChar->hardwareEnforced.clear();
    certChain_.clear();

    Status result;
    result = keymaster_->generateKey(key_desc.vector_data(), keyBlob, keyChar, &certChain_);

    // On result, blob & characteristics should be empty.
    if (!result.isOk()) {
        EXPECT_EQ(0U, keyBlob->size());
        EXPECT_EQ(0U, keyChar->hardwareEnforced.size())
                << "error = " << result.serviceSpecificErrorCode();
        EXPECT_EQ(0U, keyChar->softwareEnforced.size())
                << "error = " << result.serviceSpecificErrorCode();
        EXPECT_EQ(0U, certChain_.size());
    } else {
        if (SecLevel() != SecurityLevel::SOFTWARE) {
            EXPECT_GT(keyChar->hardwareEnforced.size(), 0);
        }
        EXPECT_GT(keyChar->softwareEnforced.size(), 0);
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::GenerateKey(const AuthorizationSet& key_desc) {
    return GenerateKey(key_desc, &key_blob_, &key_characteristics_);
}

ErrorCode KeymasterAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                           const string& key_material, vector<uint8_t>* key_blob,
                                           KeyCharacteristics* key_characteristics) {
    Status result;

    certChain_.clear();
    key_characteristics->softwareEnforced.clear();
    key_characteristics->hardwareEnforced.clear();
    key_blob->clear();

    result = keymaster_->importKey(key_desc.vector_data(), format,
                                   vector<uint8_t>(key_material.begin(), key_material.end()),
                                   key_blob, key_characteristics, &certChain_);

    if (!result.isOk()) {
        EXPECT_EQ(0U, key_blob->size());
        EXPECT_EQ(0U, (key_characteristics->softwareEnforced.size() +
                       key_characteristics->hardwareEnforced.size()));
        EXPECT_EQ(0U, certChain_.size());
    } else {
        if (SecLevel() != SecurityLevel::SOFTWARE) {
            EXPECT_GT(key_characteristics->hardwareEnforced.size(), 0);
        }
        EXPECT_GT(key_characteristics->softwareEnforced.size(), 0);
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::ImportKey(const AuthorizationSet& key_desc, KeyFormat format,
                                           const string& key_material) {
    return ImportKey(key_desc, format, key_material, &key_blob_, &key_characteristics_);
}

ErrorCode KeymasterAidlTestBase::ImportWrappedKey(string wrapped_key, string wrapping_key,
                                                  const AuthorizationSet& wrapping_key_desc,
                                                  string masking_key,
                                                  const AuthorizationSet& unwrapping_params) {
    Status result;
    EXPECT_EQ(ErrorCode::OK, ImportKey(wrapping_key_desc, KeyFormat::PKCS8, wrapping_key));

    vector<uint8_t> outBlob;
    key_characteristics_.softwareEnforced.clear();
    key_characteristics_.hardwareEnforced.clear();

    result = keymaster_->importWrappedKey(vector<uint8_t>(wrapped_key.begin(), wrapped_key.end()),
                                          key_blob_,
                                          vector<uint8_t>(masking_key.begin(), masking_key.end()),
                                          unwrapping_params.vector_data(), 0 /* passwordSid */,
                                          0 /* biometricSid */, &outBlob, &key_characteristics_);

    if (!result.isOk()) {
        EXPECT_EQ(0U, outBlob.size());
        EXPECT_EQ(0U, (key_characteristics_.softwareEnforced.size() +
                       key_characteristics_.hardwareEnforced.size()));
    } else {
        key_blob_ = outBlob;
        if (SecLevel() != SecurityLevel::SOFTWARE) {
            EXPECT_GT(key_characteristics_.hardwareEnforced.size(), 0);
        }
        EXPECT_GT(key_characteristics_.softwareEnforced.size(), 0);
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::DeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    Status result = keymaster_->deleteKey(*key_blob);
    if (!keep_key_blob) {
        *key_blob = vector<uint8_t>();
    }

    EXPECT_TRUE(result.isOk()) << result.serviceSpecificErrorCode() << endl;
    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::DeleteKey(bool keep_key_blob) {
    return static_cast<ErrorCode>(DeleteKey(&key_blob_, keep_key_blob));
}

ErrorCode KeymasterAidlTestBase::DeleteAllKeys() {
    Status result = keymaster_->deleteAllKeys();
    EXPECT_TRUE(result.isOk()) << result.serviceSpecificErrorCode() << endl;
    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

void KeymasterAidlTestBase::CheckedDeleteKey(vector<uint8_t>* key_blob, bool keep_key_blob) {
    ErrorCode result = DeleteKey(key_blob, keep_key_blob);
    EXPECT_TRUE(result == ErrorCode::OK || result == ErrorCode::UNIMPLEMENTED) << result << endl;
}

void KeymasterAidlTestBase::CheckedDeleteKey() {
    CheckedDeleteKey(&key_blob_);
}

ErrorCode KeymasterAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                       const AuthorizationSet& in_params,
                                       AuthorizationSet* out_params, sp<IKeymasterOperation>& op) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;
    result = keymaster_->begin(purpose, key_blob, in_params.vector_data(), HardwareAuthToken(),
                               &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op = out.operation;
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::Begin(KeyPurpose purpose, const vector<uint8_t>& key_blob,
                                       const AuthorizationSet& in_params,
                                       AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    Status result;
    BeginResult out;

    result = keymaster_->begin(purpose, key_blob, in_params.vector_data(), HardwareAuthToken(),
                               &out);

    if (result.isOk()) {
        *out_params = out.params;
        challenge_ = out.challenge;
        op_ = out.operation;
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params,
                                       AuthorizationSet* out_params) {
    SCOPED_TRACE("Begin");
    EXPECT_EQ(nullptr, op_);
    return Begin(purpose, key_blob_, in_params, out_params);
}

ErrorCode KeymasterAidlTestBase::Begin(KeyPurpose purpose, const AuthorizationSet& in_params) {
    SCOPED_TRACE("Begin");
    AuthorizationSet out_params;
    ErrorCode result = Begin(purpose, in_params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return result;
}

ErrorCode KeymasterAidlTestBase::Update(const AuthorizationSet& in_params, const string& input,
                                        AuthorizationSet* out_params, string* output,
                                        int32_t* input_consumed) {
    SCOPED_TRACE("Update");

    Status result;
    vector<KeyParameter> oParams;
    vector<uint8_t> oPut;

    if (!op_) {
        ALOGE("op_ null!!!!!!!!!!!!!!!!!!!!!!!!!!");
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    result = op_->update(in_params.vector_data(), vector<uint8_t>(input.begin(), input.end()),
                         VerificationToken(), &oParams, &oPut, input_consumed);

    if (result.isOk()) {
        out_params->push_back(AuthorizationSet(oParams));
        output->append(oPut.begin(), oPut.end());
    }

    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::Update(const string& input, string* out, int32_t* input_consumed) {
    SCOPED_TRACE("Update");
    AuthorizationSet out_params;
    ErrorCode result =
            Update(AuthorizationSet() /* in_params */, input, &out_params, out, input_consumed);
    EXPECT_TRUE(out_params.empty());
    return result;
}

ErrorCode KeymasterAidlTestBase::Finish(const AuthorizationSet& in_params, const string& input,
                                        const string& signature, AuthorizationSet* out_params,
                                        string* output) {
    SCOPED_TRACE("Finish");
    Status result;
    vector<uint8_t> oPut;

    if (!op_) {
        ALOGE("op_ null!!!!!!!!!!!!!!!!!!!!!!!!!!");
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    vector<KeyParameter> oParams;
    result = op_->finish(in_params.vector_data(), vector<uint8_t>(input.begin(), input.end()),
                         vector<uint8_t>(signature.begin(), signature.end()), HardwareAuthToken(),
                         VerificationToken(), &oParams, &oPut);

    if (result.isOk()) {
        output->append(oPut.begin(), oPut.end());
        *out_params = oParams;
    }

    op_.clear();  // So dtor doesn't Abort().
    return static_cast<ErrorCode>(result.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::Finish(const string& message, string* output) {
    SCOPED_TRACE("Finish");
    AuthorizationSet out_params;
    string finish_output;
    ErrorCode result = Finish(AuthorizationSet() /* in_params */, message, "" /* signature */,
                              &out_params, output);
    if (result != ErrorCode::OK) {
        return result;
    }
    EXPECT_EQ(0U, out_params.size());
    return result;
}

ErrorCode KeymasterAidlTestBase::Finish(const string& message, const string& signature,
                                        string* output) {
    SCOPED_TRACE("Finish");
    AuthorizationSet out_params;
    ErrorCode result =
            Finish(AuthorizationSet() /* in_params */, message, signature, &out_params, output);
    op_.clear();  // So dtor doesn't Abort().

    if (result != ErrorCode::OK) {
        return result;
    }

    EXPECT_EQ(0U, out_params.size());
    return result;
}

ErrorCode KeymasterAidlTestBase::Abort(sp<IKeymasterOperation>& op) {
    SCOPED_TRACE("Abort");

    if (!op) {
        ALOGE("op_ null!!!!!!!!!!!!!!!!!!!!!!!!!!");
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    Status retval = op->abort();
    EXPECT_TRUE(retval.isOk());
    return static_cast<ErrorCode>(retval.serviceSpecificErrorCode());
}

ErrorCode KeymasterAidlTestBase::Abort() {
    SCOPED_TRACE("Abort");

    if (!op_) {
        ALOGE("op_ null!!!!!!!!!!!!!!!!!!!!!!!!!!");
        return ErrorCode::UNEXPECTED_NULL_POINTER;
    }

    Status retval = op_->abort();
    return static_cast<ErrorCode>(retval.serviceSpecificErrorCode());
}

void KeymasterAidlTestBase::AbortIfNeeded() {
    SCOPED_TRACE("AbortIfNeeded");
    if (op_) {
        EXPECT_EQ(ErrorCode::OK, Abort());
        op_.clear();
    }
}

string KeymasterAidlTestBase::ProcessMessage(const vector<uint8_t>& key_blob, KeyPurpose operation,
                                             const string& message,
                                             const AuthorizationSet& in_params,
                                             AuthorizationSet* out_params) {
    SCOPED_TRACE("ProcessMessage");
    AuthorizationSet begin_out_params;
    EXPECT_EQ(ErrorCode::OK, Begin(operation, key_blob, in_params, &begin_out_params));

    string output;
    int32_t consumed = 0;
    AuthorizationSet update_params;
    AuthorizationSet update_out_params;
    EXPECT_EQ(ErrorCode::OK,
              Update(update_params, message, &update_out_params, &output, &consumed));

    string unused;
    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK,
              Finish(finish_params, message.substr(consumed), unused, &finish_out_params, &output));
    op_.clear();

    out_params->push_back(begin_out_params);
    out_params->push_back(finish_out_params);
    return output;
}

string KeymasterAidlTestBase::SignMessage(const vector<uint8_t>& key_blob, const string& message,
                                          const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    AuthorizationSet out_params;
    string signature = ProcessMessage(key_blob, KeyPurpose::SIGN, message, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return signature;
}

string KeymasterAidlTestBase::SignMessage(const string& message, const AuthorizationSet& params) {
    SCOPED_TRACE("SignMessage");
    return SignMessage(key_blob_, message, params);
}

string KeymasterAidlTestBase::MacMessage(const string& message, Digest digest, size_t mac_length) {
    SCOPED_TRACE("MacMessage");
    return SignMessage(
            key_blob_, message,
            AuthorizationSetBuilder().Digest(digest).Authorization(TAG_MAC_LENGTH, mac_length));
}

void KeymasterAidlTestBase::CheckHmacTestVector(const string& key, const string& message,
                                                Digest digest, const string& expected_mac) {
    SCOPED_TRACE("CheckHmacTestVector");
    ASSERT_EQ(ErrorCode::OK,
              ImportKey(AuthorizationSetBuilder()
                                .Authorization(TAG_NO_AUTH_REQUIRED)
                                .HmacKey(key.size() * 8)
                                .Authorization(TAG_MIN_MAC_LENGTH, expected_mac.size() * 8)
                                .Digest(digest),
                        KeyFormat::RAW, key));
    string signature = MacMessage(message, digest, expected_mac.size() * 8);
    EXPECT_EQ(expected_mac, signature)
            << "Test vector didn't match for key of size " << key.size() << " message of size "
            << message.size() << " and digest " << digest;
    CheckedDeleteKey();
}

void KeymasterAidlTestBase::CheckAesCtrTestVector(const string& key, const string& nonce,
                                                  const string& message,
                                                  const string& expected_ciphertext) {
    SCOPED_TRACE("CheckAesCtrTestVector");
    ASSERT_EQ(ErrorCode::OK, ImportKey(AuthorizationSetBuilder()
                                               .Authorization(TAG_NO_AUTH_REQUIRED)
                                               .AesEncryptionKey(key.size() * 8)
                                               .BlockMode(BlockMode::CTR)
                                               .Authorization(TAG_CALLER_NONCE)
                                               .Padding(PaddingMode::NONE),
                                       KeyFormat::RAW, key));

    auto params = AuthorizationSetBuilder()
                          .Authorization(TAG_NONCE, nonce.data(), nonce.size())
                          .BlockMode(BlockMode::CTR)
                          .Padding(PaddingMode::NONE);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(key_blob_, message, params, &out_params);
    EXPECT_EQ(expected_ciphertext, ciphertext);
}

void KeymasterAidlTestBase::CheckTripleDesTestVector(KeyPurpose purpose, BlockMode block_mode,
                                                     PaddingMode padding_mode, const string& key,
                                                     const string& iv, const string& input,
                                                     const string& expected_output) {
    auto authset = AuthorizationSetBuilder()
                           .TripleDesEncryptionKey(key.size() * 7)
                           .BlockMode(block_mode)
                           .Authorization(TAG_NO_AUTH_REQUIRED)
                           .Padding(padding_mode);
    if (iv.size()) authset.Authorization(TAG_CALLER_NONCE);
    ASSERT_EQ(ErrorCode::OK, ImportKey(authset, KeyFormat::RAW, key));
    auto begin_params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding_mode);
    if (iv.size()) begin_params.Authorization(TAG_NONCE, iv.data(), iv.size());
    AuthorizationSet output_params;
    string output = ProcessMessage(key_blob_, purpose, input, begin_params, &output_params);
    EXPECT_EQ(expected_output, output);
}

void KeymasterAidlTestBase::VerifyMessage(const vector<uint8_t>& key_blob, const string& message,
                                          const string& signature, const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    AuthorizationSet begin_out_params;
    ASSERT_EQ(ErrorCode::OK, Begin(KeyPurpose::VERIFY, key_blob, params, &begin_out_params));

    string output;
    AuthorizationSet update_params;
    AuthorizationSet update_out_params;
    int32_t consumed;
    ASSERT_EQ(ErrorCode::OK,
              Update(update_params, message, &update_out_params, &output, &consumed));
    EXPECT_TRUE(output.empty());
    EXPECT_GT(consumed, 0U);

    string unused;
    AuthorizationSet finish_params;
    AuthorizationSet finish_out_params;
    EXPECT_EQ(ErrorCode::OK, Finish(finish_params, message.substr(consumed), signature,
                                    &finish_out_params, &output));
    op_.clear();
    EXPECT_TRUE(output.empty());
}

void KeymasterAidlTestBase::VerifyMessage(const string& message, const string& signature,
                                          const AuthorizationSet& params) {
    SCOPED_TRACE("VerifyMessage");
    VerifyMessage(key_blob_, message, signature, params);
}

string KeymasterAidlTestBase::EncryptMessage(const vector<uint8_t>& key_blob, const string& message,
                                             const AuthorizationSet& in_params,
                                             AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return ProcessMessage(key_blob, KeyPurpose::ENCRYPT, message, in_params, out_params);
}

string KeymasterAidlTestBase::EncryptMessage(const string& message, const AuthorizationSet& params,
                                             AuthorizationSet* out_params) {
    SCOPED_TRACE("EncryptMessage");
    return EncryptMessage(key_blob_, message, params, out_params);
}

string KeymasterAidlTestBase::EncryptMessage(const string& message,
                                             const AuthorizationSet& params) {
    SCOPED_TRACE("EncryptMessage");
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeymasterAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                             PaddingMode padding) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_TRUE(out_params.empty()) << "Output params should be empty. Contained: " << out_params;
    return ciphertext;
}

string KeymasterAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                             PaddingMode padding, vector<uint8_t>* iv_out) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder().BlockMode(block_mode).Padding(padding);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    EXPECT_EQ(1U, out_params.size());
    auto ivVal = out_params.GetTagValue(TAG_NONCE);
    EXPECT_TRUE(ivVal.isOk());
    if (ivVal.isOk()) *iv_out = ivVal.value();
    return ciphertext;
}

string KeymasterAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                             PaddingMode padding, const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeymasterAidlTestBase::EncryptMessage(const string& message, BlockMode block_mode,
                                             PaddingMode padding, uint8_t mac_length_bits,
                                             const vector<uint8_t>& iv_in) {
    SCOPED_TRACE("EncryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding)
                          .Authorization(TAG_MAC_LENGTH, mac_length_bits)
                          .Authorization(TAG_NONCE, iv_in);
    AuthorizationSet out_params;
    string ciphertext = EncryptMessage(message, params, &out_params);
    return ciphertext;
}

string KeymasterAidlTestBase::DecryptMessage(const vector<uint8_t>& key_blob,
                                             const string& ciphertext,
                                             const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    AuthorizationSet out_params;
    string plaintext =
            ProcessMessage(key_blob, KeyPurpose::DECRYPT, ciphertext, params, &out_params);
    EXPECT_TRUE(out_params.empty());
    return plaintext;
}

string KeymasterAidlTestBase::DecryptMessage(const string& ciphertext,
                                             const AuthorizationSet& params) {
    SCOPED_TRACE("DecryptMessage");
    return DecryptMessage(key_blob_, ciphertext, params);
}

string KeymasterAidlTestBase::DecryptMessage(const string& ciphertext, BlockMode block_mode,
                                             PaddingMode padding_mode, const vector<uint8_t>& iv) {
    SCOPED_TRACE("DecryptMessage");
    auto params = AuthorizationSetBuilder()
                          .BlockMode(block_mode)
                          .Padding(padding_mode)
                          .Authorization(TAG_NONCE, iv);
    return DecryptMessage(key_blob_, ciphertext, params);
}

std::pair<ErrorCode, vector<uint8_t>> KeymasterAidlTestBase::UpgradeKey(
        const vector<uint8_t>& key_blob) {
    std::pair<ErrorCode, vector<uint8_t>> retval;
    vector<uint8_t> outKeyBlob;
    Status result = keymaster_->upgradeKey(key_blob, vector<KeyParameter>(), &outKeyBlob);
    ErrorCode errorcode = static_cast<ErrorCode>(result.serviceSpecificErrorCode());
    retval = std::tie(errorcode, outKeyBlob);

    return retval;
}
vector<uint32_t> KeymasterAidlTestBase::ValidKeySizes(Algorithm algorithm) {
    switch (algorithm) {
        case Algorithm::RSA:
            switch (SecLevel()) {
                case SecurityLevel::SOFTWARE:
                case SecurityLevel::TRUSTED_ENVIRONMENT:
                    return {2048, 3072, 4096};
                case SecurityLevel::STRONGBOX:
                    return {2048};
                default:
                    ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
                    break;
            }
            break;
        case Algorithm::EC:
            switch (SecLevel()) {
                case SecurityLevel::SOFTWARE:
                case SecurityLevel::TRUSTED_ENVIRONMENT:
                    return {224, 256, 384, 521};
                case SecurityLevel::STRONGBOX:
                    return {256};
                default:
                    ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
                    break;
            }
            break;
        case Algorithm::AES:
            return {128, 256};
        case Algorithm::TRIPLE_DES:
            return {168};
        case Algorithm::HMAC: {
            vector<uint32_t> retval((512 - 64) / 8 + 1);
            uint32_t size = 64 - 8;
            std::generate(retval.begin(), retval.end(), [&]() { return (size += 8); });
            return retval;
        }
        default:
            ADD_FAILURE() << "Invalid Algorithm: " << algorithm;
            return {};
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

vector<uint32_t> KeymasterAidlTestBase::InvalidKeySizes(Algorithm algorithm) {
    if (SecLevel() == SecurityLevel::STRONGBOX) {
        switch (algorithm) {
            case Algorithm::RSA:
                return {3072, 4096};
            case Algorithm::EC:
                return {224, 384, 521};
            case Algorithm::AES:
                return {192};
            default:
                return {};
        }
    }
    return {};
}

vector<EcCurve> KeymasterAidlTestBase::ValidCurves() {
    if (securityLevel_ == SecurityLevel::STRONGBOX) {
        return {EcCurve::P_256};
    } else {
        return {EcCurve::P_224, EcCurve::P_256, EcCurve::P_384, EcCurve::P_521};
    }
}

vector<EcCurve> KeymasterAidlTestBase::InvalidCurves() {
    if (SecLevel() == SecurityLevel::TRUSTED_ENVIRONMENT) return {};
    CHECK(SecLevel() == SecurityLevel::STRONGBOX);
    return {EcCurve::P_224, EcCurve::P_384, EcCurve::P_521};
}

vector<Digest> KeymasterAidlTestBase::ValidDigests(bool withNone, bool withMD5) {
    switch (SecLevel()) {
        case SecurityLevel::SOFTWARE:
        case SecurityLevel::TRUSTED_ENVIRONMENT:
            if (withNone) {
                if (withMD5)
                    return {Digest::NONE,      Digest::MD5,       Digest::SHA1,
                            Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
                else
                    return {Digest::NONE,      Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
            } else {
                if (withMD5)
                    return {Digest::MD5,       Digest::SHA1,      Digest::SHA_2_224,
                            Digest::SHA_2_256, Digest::SHA_2_384, Digest::SHA_2_512};
                else
                    return {Digest::SHA1, Digest::SHA_2_224, Digest::SHA_2_256, Digest::SHA_2_384,
                            Digest::SHA_2_512};
            }
            break;
        case SecurityLevel::STRONGBOX:
            if (withNone)
                return {Digest::NONE, Digest::SHA_2_256};
            else
                return {Digest::SHA_2_256};
            break;
        default:
            ADD_FAILURE() << "Invalid security level " << uint32_t(SecLevel());
            break;
    }
    ADD_FAILURE() << "Should be impossible to get here";
    return {};
}

vector<Digest> KeymasterAidlTestBase::InvalidDigests() {
    return {};
}

}  // namespace keymaster
}  // namespace hardware
}  // namespace android
