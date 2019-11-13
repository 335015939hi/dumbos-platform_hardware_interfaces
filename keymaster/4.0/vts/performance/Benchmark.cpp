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

#define LOG_TAG "keymaster_benchmark"

#include <android/hardware/keymaster/4.0/IKeymasterDevice.h>
#include <android/hardware/keymaster/4.0/types.h>
#include <keymasterV4_0/authorization_set.h>
#include <keymaster/keymaster_configuration.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <binder/IServiceManager.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#include <log/log.h>
#include <utils/StrongPointer.h>

#include <benchmark/benchmark.h>
#include <hidl/Status.h>


namespace android {
namespace hardware {
namespace keymaster {
namespace V4_0 {
namespace test {


// libutils:
using android::OK;
using android::sp;
using android::status_t;

// libhidl:
using android::hardware::Return;
using android::hardware::Void;
using android::hardware::hidl_vec;

//IKeymaster
using android::hardware::keymaster::V4_0::IKeymasterDevice;
using android::hardware::keymaster::V4_0::SecurityLevel;
using android::hardware::keymaster::V4_0::ErrorCode;
using android::hardware::keymaster::V4_0::BlockMode;
using android::hardware::hidl_string;
using android::IServiceManager;
using android::hardware::keymaster::V4_0::KeyCharacteristics;
using android::hardware::keymaster::V4_0::AuthorizationSet;
using android::hardware::keymaster::V4_0::AuthorizationSetBuilder;

// Standard library
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::unique_ptr;
using std::vector;
using std::optional;



class HidlBuf : public hidl_vec<uint8_t> {
    typedef hidl_vec<uint8_t> super;

   public:
    HidlBuf() {}
    HidlBuf(const super& other) : super(other) {}
    HidlBuf(super&& other) : super(std::move(other)) {}
    explicit HidlBuf(const std::string& other) : HidlBuf() { *this = other; }

    HidlBuf& operator=(const super& other) {
        super::operator=(other);
        return *this;
    }

    HidlBuf& operator=(super&& other) {
        super::operator=(std::move(other));
        return *this;
    }

    HidlBuf& operator=(const string& other) {
        resize(other.size());
        std::copy(other.begin(), other.end(), begin());
        return *this;
    }

    string to_string() const { return string(reinterpret_cast<const char*>(data()), size()); }
};

class KeymasterWrapper {
  private:
    sp<IKeymasterDevice> keymaster_;
    SecurityLevel securityLevel_;
    hidl_string name_;
    hidl_string author_;
    HidlBuf  key_blob_;
    KeyCharacteristics key_characteristics_;
    ErrorCode error_;
    string key_transform_;
    string keymaster_name_;
    uint32_t os_version_;
    uint32_t os_patch_level_;

    bool GenerateKey(const AuthorizationSet& authSet) {
      return (keymaster_->generateKey(authSet.hidl_data(),
                              [&](ErrorCode hidl_error, const hidl_vec<uint8_t>& hidl_key_blob,
                                        const KeyCharacteristics& hidl_key_characteristics) {
                                        error_ = hidl_error;
                                        key_blob_ = hidl_key_blob;
                                        key_characteristics_ = std::move(hidl_key_characteristics);
                                  }
        ).isOk() && error_ == ErrorCode::OK);
    }

    bool GenerateKey(Algorithm algorithm, int keySize, Digest digest = Digest::NONE, PaddingMode padding = PaddingMode::NONE, optional<BlockMode> blockMode = {}) {
      AuthorizationSetBuilder authSet = AuthorizationSetBuilder()
                            .Authorization(TAG_NO_AUTH_REQUIRED)
                            .Authorization(TAG_PURPOSE, KeyPurpose::ENCRYPT)
                            .Authorization(TAG_PURPOSE, KeyPurpose::DECRYPT)
                            .Authorization(TAG_PURPOSE, KeyPurpose::SIGN)
                            .Authorization(TAG_PURPOSE, KeyPurpose::VERIFY)
                            .Authorization(TAG_KEY_SIZE, keySize)
                            .Authorization(TAG_ALGORITHM, algorithm)
                            .Digest(digest)
                            .Authorization(TAG_MIN_MAC_LENGTH, 128)
                            .Padding(padding);
      if(blockMode) {
        authSet.BlockMode(*blockMode);
      }
      if (algorithm == Algorithm::RSA) {
        authSet.Authorization(TAG_RSA_PUBLIC_EXPONENT, 65537U);
      }
      return GenerateKey(authSet);
    }

  public:

    KeymasterWrapper(const string keymaster_name) {
      os_version_ = ::keymaster::GetOsVersion();
      os_patch_level_ = ::keymaster::GetOsPatchlevel();
      keymaster_name_ = keymaster_name;
      keymaster_ = IKeymasterDevice::getService(keymaster_name_);
      keymaster_
          ->getHardwareInfo([&](SecurityLevel securityLevel, const hidl_string& name,
                                const hidl_string& author) {
              securityLevel_ = securityLevel;
              name_ = name;
              author_ = author;
          });
    }

    bool GenerateKey(string transform, bool sign = false) {
      if(transform == key_transform_) {
        return true;
      }
      else if (key_transform_ != "") {
        // Deleting old key first
        if(!DeleteKey()) {
          return false;
        }
      }
      optional<Algorithm> algorithm = getAlgorithm(transform);
      if (!algorithm) {
        cerr << "Error: invalid algorithm " << transform << endl;
        return false;
      }
      uint32_t keySize = getKeySize(transform);
      if(keySize == 0) {
        cerr << "Error: unable to determine key size " << transform << endl;
        return false;
      }
      key_transform_ = transform;
      return GenerateKey(*algorithm, keySize, getDigest(transform), getPadding(transform, sign), getBlockMode(transform));
    }

    bool DeleteKey() {
      key_blob_ = HidlBuf();
      key_transform_ = "";
      return keymaster_->deleteKey(key_blob_).isOk();
    }

    AuthorizationSet getOperationParams(string transform, bool sign = false) {
      AuthorizationSetBuilder builder = AuthorizationSetBuilder()
            .Padding(getPadding(transform, sign))
            .Authorization(TAG_MAC_LENGTH, 128)
            .Digest(getDigest(transform));
      optional<BlockMode> blockMode = getBlockMode(transform);
      if(blockMode) {
        builder.BlockMode(*blockMode);
      }
      return std::move(builder);
    }

    optional<string> Encrypt(const string& message, AuthorizationSet& in_params, AuthorizationSet* out_params = new AuthorizationSet) {
      return ProcessMessage(KeyPurpose::ENCRYPT, message, in_params, out_params);
    }

    optional<string> Decrypt(const string& message, AuthorizationSet& in_params, AuthorizationSet* out_params = new AuthorizationSet) {
      return ProcessMessage(KeyPurpose::DECRYPT, message, in_params, out_params);
    }

    optional<string> Sign(const string& message, AuthorizationSet& in_params, AuthorizationSet* out_params = new AuthorizationSet) {
      return ProcessMessage(KeyPurpose::SIGN, message, in_params, out_params);
    }

    optional<string> Verify(const string& message, const string& signature, AuthorizationSet& in_params, AuthorizationSet* out_params = new AuthorizationSet) {
      return ProcessMessage(KeyPurpose::VERIFY, message, in_params, out_params, signature);
    }

    optional<string> ProcessMessage(KeyPurpose operation, const string& message, const AuthorizationSet& in_params,
                                             AuthorizationSet* out_params, const string& signature = "") {

      static const int HIDL_BUFFER_LIMIT = 1<<14; //16KB

      OperationHandle op_handle;
      if(!keymaster_->begin(operation, key_blob_, in_params.hidl_data(), HardwareAuthToken(),
                              [&](ErrorCode hidl_error, const hidl_vec<KeyParameter>& hidl_out_params,
                                  uint64_t hidl_op_handle) {
                                  error_ = hidl_error;
                                  out_params->push_back(AuthorizationSet(hidl_out_params));
                                  op_handle = hidl_op_handle;
                              })
                      .isOk() || error_ != ErrorCode::OK) {
        keymaster_->abort(op_handle);
        return {};
      }


      string output;
      size_t input_consumed = 0;
      while(message.length()-input_consumed > 0) {
        if(!keymaster_->update(op_handle, in_params.hidl_data(), HidlBuf(message.substr(input_consumed, HIDL_BUFFER_LIMIT)),
                            HardwareAuthToken(), VerificationToken(),
                             [&](ErrorCode hidl_error, uint32_t hidl_input_consumed,
                                 const hidl_vec<KeyParameter>& hidl_out_params,
                                 const HidlBuf& hidl_output) {
                                 error_ = hidl_error;
                                 out_params->push_back(AuthorizationSet(hidl_out_params));
                                 output.append(hidl_output.to_string());
                                 input_consumed += hidl_input_consumed;
                             }
                        ).isOk() || error_ != ErrorCode::OK) {
          keymaster_->abort(op_handle);
          return {};
        }
      }

      if(!keymaster_->finish(op_handle, in_params.hidl_data(), HidlBuf(message.substr(input_consumed)), HidlBuf(signature),
                     HardwareAuthToken(), VerificationToken(),
                     [&](ErrorCode hidl_error, const hidl_vec<KeyParameter>& hidl_out_params,
                         const HidlBuf& hidl_output) {
                         error_ = hidl_error;
                         out_params->push_back(AuthorizationSet(hidl_out_params));
                         output.append(hidl_output.to_string());
                     }).isOk() || error_ != ErrorCode::OK) {
        keymaster_->abort(op_handle);
        return {};
      }

      return output;
    }

    int getError() {
      return static_cast<int>(error_);
    }

    const string& GenerateMessage(int size) {
      return std::move(string("x", size));
    }

    optional<BlockMode> getBlockMode(string transform) {
      if(transform.find("/ECB") != string::npos) {
          return BlockMode::ECB;
      }
      else if(transform.find("/CBC") != string::npos) {
          return BlockMode::CBC;
      }
      else if(transform.find("/CTR") != string::npos) {
          return BlockMode::CTR;
      }
      else if(transform.find("/GCM") != string::npos) {
          return BlockMode::GCM;
      }
      return {};
    }

    uint32_t getKeySize(string transform) {
      return std::stoi(transform.substr(transform.rfind('/')+1));
    }

    PaddingMode getPadding(string transform, bool sign) {
      if(transform.find("/PKCS7") != string::npos) {
        return PaddingMode::PKCS7;
      }
      else if(transform.find("/PSS") != string::npos) {
        return PaddingMode::RSA_PSS;
      }
      else if(transform.find("/OAEP") != string::npos) {
        return PaddingMode::RSA_OAEP;
      }
      else if(transform.find("/PKCS1") != string::npos) {
        return sign ? PaddingMode::RSA_PKCS1_1_5_SIGN : PaddingMode::RSA_PKCS1_1_5_ENCRYPT;
      }
      return PaddingMode::NONE;
    }

    optional<Algorithm> getAlgorithm(string transform) {
      if(transform.find("AES") != string::npos) {
        return Algorithm::AES;
      }
      else if(transform.find("Hmac") != string::npos) {
        return Algorithm::HMAC;
      }
      else if(transform.find("DESede") != string::npos) {
        return Algorithm::TRIPLE_DES;
      }
      else if(transform.find("RSA") != string::npos) {
        return Algorithm::RSA;
      }
      else if(transform.find("EC") != string::npos) {
        return Algorithm::EC;
      }
      cerr << "Can't find algorithm for " << transform << endl;
      return {};
    }

    Digest getDigest(string transform){
      if(transform.find("MD5") != string::npos) {
        return Digest::MD5;
      }
      else if(transform.find("SHA1") != string::npos || transform.find("SHA-1") != string::npos) {
        return Digest::SHA1;
      }
      else if(transform.find("SHA224") != string::npos) {
          return Digest::SHA_2_224;
      }
      else if(transform.find("SHA256") != string::npos) {
          return Digest::SHA_2_256;
      }
      else if(transform.find("SHA384") != string::npos) {
          return Digest::SHA_2_384;
      }
      else if(transform.find("SHA512") != string::npos) {
          return Digest::SHA_2_512;
      }
      return Digest::NONE;
    }

};

KeymasterWrapper *keymaster;

static const string SMALL_MESSAGE = string("x", 64);
static const string MEDIUM_MESSAGE = string("x", 1<<10); //1KB
static const string LARGE_MESSAGE = string("x", 1<<17); //128KB


static void settings(benchmark::internal::Benchmark *benchmark) {
  benchmark->Unit(benchmark::kMillisecond);
}
#define BENCHMARK_KM(func, transform, ...) BENCHMARK_CAPTURE(func, transform, #transform, ##__VA_ARGS__)->Apply(settings);

#define BENCHMARK_KM_MSGS(func, transform, ...) \
    BENCHMARK_KM(func, transform, SMALL_MESSAGE, ##__VA_ARGS__) \
    BENCHMARK_KM(func, transform, MEDIUM_MESSAGE, ##__VA_ARGS__) \
    BENCHMARK_KM(func, transform, LARGE_MESSAGE, ##__VA_ARGS__)

#define BENCHMARK_KM_CIPHER(transform, ...) BENCHMARK_KM(encrypt, transform, ##__VA_ARGS__) \
                                            BENCHMARK_KM(decrypt, transform, ##__VA_ARGS__)

#define BENCHMARK_KM_CIPHER_MSGS(transform, ...) BENCHMARK_KM_MSGS(encrypt, transform, ##__VA_ARGS__) \
                                                 BENCHMARK_KM_MSGS(decrypt, transform, ##__VA_ARGS__)

#define BENCHMARK_KM_SIGNATURE_MSGS(transform, ...) BENCHMARK_KM_MSGS(sign, transform, ##__VA_ARGS__) \
                                                    BENCHMARK_KM_MSGS(verify, transform, ##__VA_ARGS__)


/*
 * ============= KeyGen TESTS ==================
 */
static void keygen(benchmark::State& state, string transform) {
  for(auto _ : state) {
    keymaster->GenerateKey(transform);
    state.PauseTiming();
    keymaster->DeleteKey();
    state.ResumeTiming();
  }
}


BENCHMARK_KM(keygen, AES/128);
BENCHMARK_KM(keygen, AES/192);
BENCHMARK_KM(keygen, AES/256);

BENCHMARK_KM(keygen, RSA/512);
BENCHMARK_KM(keygen, RSA/1024);
BENCHMARK_KM(keygen, RSA/2048);
BENCHMARK_KM(keygen, RSA/3072);
BENCHMARK_KM(keygen, RSA/4096);

BENCHMARK_KM(keygen, EC/224);
BENCHMARK_KM(keygen, EC/256);
BENCHMARK_KM(keygen, EC/384);
BENCHMARK_KM(keygen, EC/521);

BENCHMARK_KM(keygen, DESede/168);

BENCHMARK_KM(keygen, Hmac/64);
BENCHMARK_KM(keygen, Hmac/128);
BENCHMARK_KM(keygen, Hmac/256);
BENCHMARK_KM(keygen, Hmac/512);
BENCHMARK_KM(keygen, Hmac/1024);
BENCHMARK_KM(keygen, Hmac/2048);
BENCHMARK_KM(keygen, Hmac/4096);
BENCHMARK_KM(keygen, Hmac/8192);

/*
 * ============= SIGNATURE TESTS ==================
 */

static void sign(benchmark::State& state, string transform, const string& message) {
  state.SetLabel("msgSize:"+std::to_string(message.size()));
  if(!keymaster->GenerateKey(transform, true)) {
    state.SkipWithError(("Key generation error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  auto params = keymaster->getOperationParams(transform, true);
  for(auto _ : state) {
    if(!keymaster->Sign(message, params)) {
      state.SkipWithError(("Sign error, " + std::to_string(keymaster->getError())).c_str());
      break;
    }
  }
}

static void verify(benchmark::State& state, string transform, const string& message) {
  state.SetLabel("msgSize:"+std::to_string(message.size()));
  if(!keymaster->GenerateKey(transform, true)) {
    state.SkipWithError(("Key generation error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  AuthorizationSet out_params;
  AuthorizationSet in_params = keymaster->getOperationParams(transform, true);
  optional<string> signature = keymaster->Sign(message, in_params, &out_params);
  if(!signature) {
    state.SkipWithError(("Sign error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  in_params.push_back(out_params);
  for(auto _ : state) {
    if(!keymaster->Verify(message, *signature, in_params)) {
      state.SkipWithError(("Verify error, " + std::to_string(keymaster->getError())).c_str());
      break;
    }
  }
}

#define BENCHMARK_KM_HMAC_SIGNATURE(transform) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/64) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/128) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/256) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/512) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/1024) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/2024) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/4096) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/8192)

BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA1)
BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA256)
BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA224)
BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA256)
BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA384)
BENCHMARK_KM_HMAC_SIGNATURE(HmacSHA512)

#define BENCHMARK_KM_ECDSA_SIGNATURE(transform) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/224) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/256) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/384) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/521)

BENCHMARK_KM_ECDSA_SIGNATURE(NONEwithECDSA);
BENCHMARK_KM_ECDSA_SIGNATURE(SHA1withECDSA);
BENCHMARK_KM_ECDSA_SIGNATURE(SHA224withECDSA);
BENCHMARK_KM_ECDSA_SIGNATURE(SHA256withECDSA);
BENCHMARK_KM_ECDSA_SIGNATURE(SHA384withECDSA);
BENCHMARK_KM_ECDSA_SIGNATURE(SHA512withECDSA);


#define BENCHMARK_KM_RSA_SIGNATURE(transform) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/512) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/768) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/1024) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/2048) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/3072) \
  BENCHMARK_KM_SIGNATURE_MSGS(transform/4096)


BENCHMARK_KM_RSA_SIGNATURE(MD5withRSA/PKCS1);
BENCHMARK_KM_RSA_SIGNATURE(SHA1withRSA/PKCS1);
BENCHMARK_KM_RSA_SIGNATURE(SHA224withRSA/PKCS1);
BENCHMARK_KM_RSA_SIGNATURE(SHA384withRSA/PKCS1)
BENCHMARK_KM_RSA_SIGNATURE(SHA512withRSA/PKCS1);
BENCHMARK_KM_RSA_SIGNATURE(MD5withRSA/PSS);
BENCHMARK_KM_RSA_SIGNATURE(SHA1withRSA/PSS);
BENCHMARK_KM_RSA_SIGNATURE(SHA224withRSA/PSS);
BENCHMARK_KM_RSA_SIGNATURE(SHA384withRSA/PSS);
BENCHMARK_KM_RSA_SIGNATURE(SHA512withRSA/PSS);


/*
 * ============= CIPHER TESTS ==================
 */

static void encrypt(benchmark::State& state, string transform, const string& message) {
  state.SetLabel("msgSize:"+std::to_string(message.size()));
  if(!keymaster->GenerateKey(transform)) {
    state.SkipWithError(("Key generation error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  auto params = keymaster->getOperationParams(transform);
  for(auto _ : state) {
    if(!keymaster->Encrypt(message, params)) {
      state.SkipWithError(("Encryption error, " + std::to_string(keymaster->getError())).c_str());
      break;
    }
  }
}

static void decrypt(benchmark::State& state, string transform, const string& message) {
  state.SetLabel("msgSize:"+std::to_string(message.size()));
  if(!keymaster->GenerateKey(transform)) {
    state.SkipWithError(("Key generation error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  AuthorizationSet out_params;
  AuthorizationSet in_params = keymaster->getOperationParams(transform);
  optional<string> encrypted_message = keymaster->Encrypt(message, in_params, &out_params);
  if(!encrypted_message) {
    state.SkipWithError(("Encryption error, " + std::to_string(keymaster->getError())).c_str());
    return;
  }
  in_params.push_back(out_params);
  for(auto _ : state) {
    if(!keymaster->Decrypt(*encrypted_message, in_params)) {
      state.SkipWithError(("Decryption error, " + std::to_string(keymaster->getError())).c_str());
      break;
    }
  }
}

// AES
#define BENCHMARK_KM_CIPHER_ALL_AES_KEYS(transform) \
  BENCHMARK_KM_CIPHER_MSGS(transform/128) \
  BENCHMARK_KM_CIPHER_MSGS(transform/192) \
  BENCHMARK_KM_CIPHER_MSGS(transform/256)

BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CBC/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CBC/PKCS7Padding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/CTR/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/ECB/NoPadding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/ECB/PKCS7Padding);
BENCHMARK_KM_CIPHER_ALL_AES_KEYS(AES/GCM/NoPadding);

// Triple DES
BENCHMARK_KM_CIPHER_MSGS(DESede/CBC/NoPadding/168);
BENCHMARK_KM_CIPHER_MSGS(DESede/CBC/PKCS7Padding/168);
BENCHMARK_KM_CIPHER_MSGS(DESede/ECB/NoPadding/168);
BENCHMARK_KM_CIPHER_MSGS(DESede/ECB/PKCS7Padding/168);

#define BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(transform, message) \
  BENCHMARK_KM_CIPHER(transform/1024, message) \
  BENCHMARK_KM_CIPHER(transform/2048, message) \
  BENCHMARK_KM_CIPHER(transform/3072, message) \
  BENCHMARK_KM_CIPHER(transform/4096, message)

BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSA/ECB/NoPadding, SMALL_MESSAGE);

// // RSA/ECB/PKCS1Padding
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSA/ECB/PKCS1Padding, SMALL_MESSAGE);

// RSA/ECB/OAEPPadding
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithMD5/ECB/OAEPPadding, SMALL_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA1/ECB/OAEPPadding, SMALL_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA224/ECB/OAEPPadding, SMALL_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA256/ECB/OAEPPadding, SMALL_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA384/ECB/OAEPPadding, SMALL_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA512/ECB/OAEPPadding, SMALL_MESSAGE);

BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithMD5/ECB/OAEPPadding, MEDIUM_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA1/ECB/OAEPPadding, MEDIUM_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA224/ECB/OAEPPadding, MEDIUM_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA256/ECB/OAEPPadding, MEDIUM_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA384/ECB/OAEPPadding, MEDIUM_MESSAGE);
BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA512/ECB/OAEPPadding, MEDIUM_MESSAGE);


BENCHMARK_KM_CIPHER_ALL_RSA_KEYS(RSAwithSHA1/ECB/OAEPPadding, LARGE_MESSAGE);

}  // namespace test
}  // namespace V4_0
}  // namespace keymaster
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
  android::hardware::keymaster::V4_0::test::keymaster = new android::hardware::keymaster::V4_0::test::KeymasterWrapper("default");
  ::benchmark::RunSpecifiedBenchmarks();
}