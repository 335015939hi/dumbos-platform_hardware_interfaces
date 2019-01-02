/*
 * Copyright 2019, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "android.hardware.identity_credential-support"

#include <android/hardware/identity_credential/support/IdentityCredentialUtils.h>

#include <ctype.h>
#include <stdio.h>

#include <openssl/aes.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

#include <keymaster/android_keymaster.h>
#include <keymaster/contexts/pure_soft_keymaster_context.h>
#include <keymaster/key.h>
#include <keymaster/km_openssl/ec_key_factory.h>
#include <keymaster/km_openssl/openssl_utils.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace support {

using std::string;
using std::vector;

using android::hardware::identity_credential::V1_0::EntryValue;

// ---------------------------------------------------------------------------
// Miscellaneous utilities.
// ---------------------------------------------------------------------------

void hexdump(const string& name, const vector<uint8_t>& data) {
    fprintf(stderr, "%s: dumping %zd bytes\n", name.c_str(), data.size());
    size_t n, m, o;
    for (n = 0; n < data.size(); n += 16) {
        fprintf(stderr, "%04zx  ", n);
        for (m = 0; m < 16 && n + m < data.size(); m++) {
            fprintf(stderr, "%02x ", data[n + m]);
        }
        for (o = m; o < 16; o++) {
            fprintf(stderr, "   ");
        }
        fprintf(stderr, " ");
        for (m = 0; m < 16 && n + m < data.size(); m++) {
            int c = data[n + m];
            fprintf(stderr, "%c", isprint(c) ? c : '.');
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

// ---------------------------------------------------------------------------
// CBOR / libcn-cbor utilities.
// ---------------------------------------------------------------------------

bool cborEncode(cn_cbor* value, vector<uint8_t>& encoded) {
    // Unfortunately there's no way to know how big the encoded blob will be [1]
    // so we just hardcode a ceiling for now... it's not very elegant but it
    // works.
    //
    // [1] : ideally cn_cbor_encoded_write(nullptr, 0, 0, array.get()) would
    // return how many bytes _would_ have been written just like sprintf() and
    // friends... but that's not how it works right now.
    encoded.resize(1024 * 1024);
    ssize_t enc_sz = cn_cbor_encoder_write(encoded.data(), 0, encoded.size(), value);
    if (enc_sz == -1) {
        LOG(ERROR) << "Error encoding CBOR data";
        return false;
    }
    encoded.resize(enc_sz);
    return true;
}

// Takes ownership of |cbor_value| on both success and error paths.
bool cborMapPutStringValue(cn_cbor* map, const char* key, cn_cbor* cbor_value) {
    cn_cbor_errback err;

    if (!cn_cbor_mapput_string(map, key, cbor_value, &err)) {
        LOG(ERROR) << "Error " << err.err << " putting value in map (pos " << err.pos << ")";
        cn_cbor_free(cbor_value);
        return false;
    }
    return true;
}

bool cborMapPutStringString(cn_cbor* map, const char* key, const char* value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_string_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating string (pos " << err.pos << ")";
        return false;
    }
    return cborMapPutStringValue(map, key, cbor_value);
}

bool cborMapPutStringBStr(cn_cbor* map, const char* key, const uint8_t* value, size_t value_size) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_data_create(value, value_size, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    return cborMapPutStringValue(map, key, cbor_value);
}

bool cborMapPutStringInt(cn_cbor* map, const char* key, int64_t value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_int_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating int (pos " << err.pos << ")";
        return false;
    }
    return cborMapPutStringValue(map, key, cbor_value);
}

bool cborMapPutStringBool(cn_cbor* map, const char* key, bool value) {
    // There is no cn_cbor_bool_create()
    cn_cbor* cbor_value = (cn_cbor*)calloc(1, sizeof(cn_cbor));
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error creating bool";
        return false;
    }
    cbor_value->type = value ? CN_CBOR_TRUE : CN_CBOR_FALSE;
    return cborMapPutStringValue(map, key, cbor_value);
}

// Takes ownership of |cbor_value| on both success and error paths.
bool cborArrayAppendValue(cn_cbor* array, cn_cbor* cbor_value) {
    cn_cbor_errback err;
    if (!cn_cbor_array_append(array, cbor_value, &err)) {
        LOG(ERROR) << "Error " << err.err << " appending value to array (pos " << err.pos << ")";
        cn_cbor_free(cbor_value);
        return false;
    }
    return true;
}

bool cborArrayAppendInt(cn_cbor* array, int64_t value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_int_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating int (pos " << err.pos << ")";
        return false;
    }
    return cborArrayAppendValue(array, cbor_value);
}

bool cborArrayAppendBool(cn_cbor* array, bool value) {
    // There is no cn_cbor_bool_create()
    cn_cbor* cbor_value = (cn_cbor*)calloc(1, sizeof(cn_cbor));
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error creating bool";
        return false;
    }
    cbor_value->type = value ? CN_CBOR_TRUE : CN_CBOR_FALSE;
    return cborArrayAppendValue(array, cbor_value);
}

bool cborArrayAppendString(cn_cbor* array, const char* value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_string_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating string (pos " << err.pos << ")";
        return false;
    }
    return cborArrayAppendValue(array, cbor_value);
}

bool cborArrayAppendBStr(cn_cbor* array, const uint8_t* value, size_t value_size) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_data_create(value, value_size, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    return cborArrayAppendValue(array, cbor_value);
}

bool cborArrayGetString(cn_cbor* array, size_t idx, std::string& data) {
    cn_cbor* elem = cn_cbor_index(array, idx);
    if (elem == nullptr) {
        LOG(ERROR) << "No element at given index";
        return false;
    }
    if (elem->type != CN_CBOR_TEXT) {
        LOG(ERROR) << "Expected type CN_CBOR_BYTES but got " << elem->type;
        return false;
    }
    data.resize(elem->length);
    memcpy(data.data(), elem->v.bytes, elem->length);
    return true;
}

bool cborArrayGetBool(cn_cbor* array, size_t idx, bool& data) {
    cn_cbor* elem = cn_cbor_index(array, idx);
    if (elem == nullptr) {
        LOG(ERROR) << "No element at given index";
        return false;
    }
    if (elem->type == CN_CBOR_TRUE) {
        data = true;
    } else if (elem->type == CN_CBOR_FALSE) {
        data = false;
    } else {
        LOG(ERROR) << "Expected type CN_CBOR_TRUE or CN_CBOR_FALSE but got " << elem->type;
        return false;
    }
    return true;
}

bool cborArrayGetBStr(cn_cbor* array, size_t idx, vector<uint8_t>& data) {
    cn_cbor* elem = cn_cbor_index(array, idx);
    if (elem == nullptr) {
        LOG(ERROR) << "No element at given index";
        return false;
    }
    if (elem->type != CN_CBOR_BYTES) {
        LOG(ERROR) << "Expected type CN_CBOR_BYTES but got " << elem->type;
        return false;
    }
    data.resize(elem->length);
    memcpy(data.data(), elem->v.bytes, elem->length);
    return true;
}

static bool cborPrettyPrintInternal(cn_cbor* value, std::string& out, size_t indent,
                                    size_t maxBStrSize) {
    char buf[80];

    switch (value->type) {
        case CN_CBOR_FALSE:
            out.append("false");
            break;
        case CN_CBOR_TRUE:
            out.append("true");
            break;
        case CN_CBOR_NULL:
            out.append("null");
            break;
        case CN_CBOR_UNDEF:
            out.append("undefined");
            break;
        case CN_CBOR_UINT:
            snprintf(buf, sizeof(buf), "%" PRIu64, (uint64_t)value->v.uint);
            out.append(buf);
            break;
        case CN_CBOR_INT:
            snprintf(buf, sizeof(buf), "%" PRId64, (uint64_t)value->v.sint);
            out.append(buf);
            break;
        case CN_CBOR_BYTES:
            out.append("{");
            if (value->length > maxBStrSize) {
                unsigned char digest[SHA_DIGEST_LENGTH];
                SHA_CTX ctx;
                SHA1_Init(&ctx);
                SHA1_Update(&ctx, value->v.bytes, value->length);
                SHA1_Final(digest, &ctx);
                char buf2[SHA_DIGEST_LENGTH * 2 + 1];
                for (size_t n = 0; n < SHA_DIGEST_LENGTH; n++) {
                    snprintf(buf2 + n * 2, 3, "%02x", digest[n]);
                }
                snprintf(buf, sizeof(buf), "<bstr size=%d sha1=%s>", value->length, buf2);
                out.append(buf);
            } else {
                for (size_t n = 0; n < value->length; n++) {
                    if (n > 0) {
                        out.append(", ");
                    }
                    snprintf(buf, sizeof(buf), "0x%02x", value->v.bytes[n]);
                    out.append(buf);
                }
            }
            out.append("}");
            break;
        case CN_CBOR_TEXT:
            out.append("'");
            {
                // TODO: escape "'" characters
                string str = string(value->v.str, value->length);
                out.append(str.c_str());
            }
            out.append("'");
            break;
        case CN_CBOR_ARRAY:
            out.append("[");
            for (size_t n = 0; n < value->length; n++) {
                if (n > 0) {
                    out.append(", ");
                }
                cn_cbor* item = cn_cbor_index(value, n);
                if (!cborPrettyPrintInternal(item, out, indent + 2, maxBStrSize)) {
                    return false;
                }
            }
            out.append("]");
            break;
        case CN_CBOR_MAP:
            out.append("{\n");
            for (size_t n = 0; n < value->length; n += 2) {
                for (size_t m = 0; m < indent + 2; m++) {
                    out.append(" ");
                }
                cn_cbor* map_key = cn_cbor_index(value, n);
                cn_cbor* map_value = cn_cbor_index(value, n + 1);
                if (!cborPrettyPrintInternal(map_key, out, indent + 2, maxBStrSize)) {
                    return false;
                }
                out.append(" : ");
                if (!cborPrettyPrintInternal(map_value, out, indent + 2, maxBStrSize)) {
                    return false;
                }
                if (n + 2 < value->length) {
                    out.append(",");
                }
                out.append("\n");
            }
            for (size_t m = 0; m < indent; m++) {
                out.append(" ");
            }
            out.append("}");
            break;
        case CN_CBOR_TAG:
            out.append("x");
            break;
        case CN_CBOR_DOUBLE:
            snprintf(buf, sizeof(buf), "%g", value->v.dbl);
            out.append(buf);
            break;
        case CN_CBOR_INVALID:
            LOG(ERROR) << "Encountered type invalid while pretty printing";
            return false;
        case CN_CBOR_SIMPLE:
        case CN_CBOR_BYTES_CHUNKED:
        case CN_CBOR_TEXT_CHUNKED:
            LOG(ERROR) << "No pretty-print support for CBOR type " << value->type;
            return false;
    }
    return true;
}

static bool cborPrettyPrintInternal(const std::vector<uint8_t>& encodedCbor, std::string& out,
                                    size_t indent, size_t maxBStrSize) {
    cn_cbor_errback err;
    auto value = CnCborPtr(cn_cbor_decode(encodedCbor.data(), encodedCbor.size(), &err));
    if (value.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " decoding CBOR (pos " << err.pos << ")";
        return false;
    }
    return cborPrettyPrintInternal(value.get(), out, indent, maxBStrSize);
}

bool cborPrettyPrint(cn_cbor* value, std::string& out, size_t maxBStrSize) {
    out.clear();
    return cborPrettyPrintInternal(value, out, 0, maxBStrSize);
}

bool cborPrettyPrint(const std::vector<uint8_t>& encodedCbor, std::string& out,
                     size_t maxBStrSize) {
    out.clear();
    return cborPrettyPrintInternal(encodedCbor, out, 0, maxBStrSize);
}

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

struct EVP_CIPHER_CTX_Deleter {
    void operator()(EVP_CIPHER_CTX* ctx) const {
        if (ctx != nullptr) {
            EVP_CIPHER_CTX_free(ctx);
        }
    }
};

using EvpCipherCtxPtr = std::unique_ptr<EVP_CIPHER_CTX, EVP_CIPHER_CTX_Deleter>;

bool getRandom(size_t numBytes, vector<uint8_t>& output) {
    output.resize(numBytes);
    if (RAND_bytes(output.data(), numBytes) != 1) {
        LOG(ERROR) << "RAND_bytes: failed getting " << numBytes << " random";
        return false;
    }
    return true;
}

bool decryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& encryptedData,
                      const vector<uint8_t>& additionalAuthenticatedData,
                      vector<uint8_t>& plainText) {
    int cipherTextSize = int(encryptedData.size()) - kAesGcmIvSize - kAesGcmTagSize;
    if (cipherTextSize < 0) {
        LOG(ERROR) << "encryptedData too small";
        return false;
    }
    unsigned char* nonce = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = nonce + kAesGcmIvSize;
    unsigned char* tag = cipherText + cipherTextSize;
    plainText.resize(cipherTextSize);

    auto ctx = EvpCipherCtxPtr(EVP_CIPHER_CTX_new());
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "EVP_CIPHER_CTX_new: failed";
        return false;
    }

    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, NULL, NULL) != 1) {
        LOG(ERROR) << "EVP_DecryptInit_ex: failed";
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kAesGcmIvSize, NULL) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting nonce length";
        return false;
    }

    if (EVP_DecryptInit_ex(ctx.get(), NULL, NULL, (unsigned char*)key.data(), nonce) != 1) {
        LOG(ERROR) << "EVP_DecryptInit_ex: failed";
        return false;
    }

    int numWritten;
    if (additionalAuthenticatedData.size() > 0) {
        if (EVP_DecryptUpdate(ctx.get(), NULL, &numWritten,
                              (unsigned char*)additionalAuthenticatedData.data(),
                              additionalAuthenticatedData.size()) != 1) {
            LOG(ERROR) << "EVP_DecryptUpdate: failed for additionalAuthenticatedData";
            return false;
        }
        if ((size_t)numWritten != additionalAuthenticatedData.size()) {
            LOG(ERROR) << "EVP_DecryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << additionalAuthenticatedData.size() << ") for additionalAuthenticatedData";
            return false;
        }
    }

    if (EVP_DecryptUpdate(ctx.get(), (unsigned char*)plainText.data(), &numWritten, cipherText,
                          cipherTextSize) != 1) {
        LOG(ERROR) << "EVP_DecryptUpdate: failed";
        return false;
    }
    if (numWritten != cipherTextSize) {
        LOG(ERROR) << "EVP_DecryptUpdate: Unexpected outl=" << numWritten << " (expected "
                   << cipherTextSize << ")";
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kAesGcmTagSize, tag)) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting expected tag";
        return false;
    }

    if (EVP_DecryptFinal_ex(ctx.get(), (unsigned char*)plainText.data() + numWritten, &numWritten) <
        0) {
        LOG(ERROR) << "EVP_DecryptFinal_ex: failed";
        return false;
    }
    if (numWritten != 0) {
        LOG(ERROR) << "EVP_DecryptFinal_ex: Unexpected non-zero outl=" << numWritten;
        return false;
    }

    return true;
}

bool encryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& nonce,
                      const vector<uint8_t>& data,
                      const vector<uint8_t>& additionalAuthenticatedData,
                      vector<uint8_t>& encryptedData) {
    if (key.size() != kAes128GcmKeySize) {
        LOG(ERROR) << "key is not kAes128GcmKeySize bytes";
        return false;
    }
    if (nonce.size() != kAesGcmIvSize) {
        LOG(ERROR) << "nonce is not kAesGcmIvSize bytes";
        return false;
    }

    // The result is the nonce (kAesGcmIvSize bytes), the ciphertext, and
    // finally the tag (kAesGcmTagSize bytes).
    encryptedData.resize(data.size() + kAesGcmIvSize + kAesGcmTagSize);
    unsigned char* noncePtr = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = noncePtr + kAesGcmIvSize;
    unsigned char* tag = cipherText + data.size();
    memcpy(noncePtr, nonce.data(), kAesGcmIvSize);

    auto ctx = EvpCipherCtxPtr(EVP_CIPHER_CTX_new());
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "EVP_CIPHER_CTX_new: failed";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, NULL, NULL) != 1) {
        LOG(ERROR) << "EVP_EncryptInit_ex: failed";
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kAesGcmIvSize, NULL) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed setting nonce length";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx.get(), NULL, NULL, (unsigned char*)key.data(),
                           (unsigned char*)nonce.data()) != 1) {
        LOG(ERROR) << "EVP_EncryptInit_ex: failed";
        return false;
    }

    int numWritten;
    if (additionalAuthenticatedData.size() > 0) {
        if (EVP_EncryptUpdate(ctx.get(), NULL, &numWritten,
                              (unsigned char*)additionalAuthenticatedData.data(),
                              additionalAuthenticatedData.size()) != 1) {
            LOG(ERROR) << "EVP_EncryptUpdate: failed for additionalAuthenticatedData";
            return false;
        }
        if ((size_t)numWritten != additionalAuthenticatedData.size()) {
            LOG(ERROR) << "EVP_EncryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << additionalAuthenticatedData.size() << ") for additionalAuthenticatedData";
            return false;
        }
    }

    if (data.size() > 0) {
        if (EVP_EncryptUpdate(ctx.get(), cipherText, &numWritten, (unsigned char*)data.data(),
                              data.size()) != 1) {
            LOG(ERROR) << "EVP_EncryptUpdate: failed";
            return false;
        }
        if ((size_t)numWritten != data.size()) {
            LOG(ERROR) << "EVP_EncryptUpdate: Unexpected outl=" << numWritten << " (expected "
                       << data.size() << ")";
            return false;
        }
    }

    if (EVP_EncryptFinal_ex(ctx.get(), cipherText + numWritten, &numWritten) != 1) {
        LOG(ERROR) << "EVP_EncryptFinal_ex: failed";
        return false;
    }
    if (numWritten != 0) {
        LOG(ERROR) << "EVP_EncryptFinal_ex: Unexpected non-zero outl=" << numWritten;
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kAesGcmTagSize, tag) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed getting tag";
        return false;
    }

    return true;
}

struct EC_KEY_Deleter {
    void operator()(EC_KEY* key) const {
        if (key != nullptr) {
            EC_KEY_free(key);
        }
    }
};

using EC_KEY_Ptr = std::unique_ptr<EC_KEY, EC_KEY_Deleter>;

struct EVP_PKEY_Deleter {
    void operator()(EVP_PKEY* key) const {
        if (key != nullptr) {
            EVP_PKEY_free(key);
        }
    }
};

using EVP_PKEY_Ptr = std::unique_ptr<EVP_PKEY, EVP_PKEY_Deleter>;

struct EC_GROUP_Deleter {
    void operator()(EC_GROUP* group) const {
        if (group != nullptr) {
            EC_GROUP_free(group);
        }
    }
};

using EC_GROUP_Ptr = std::unique_ptr<EC_GROUP, EC_GROUP_Deleter>;

bool createEcKeyAndAttestationChain(std::vector<uint8_t>& key, std::vector<uint8_t>& certificate) {
    auto ec_key = EC_KEY_Ptr(EC_KEY_new());
    auto pkey = EVP_PKEY_Ptr(EVP_PKEY_new());
    // TODO: maybe allow specifying curve instead of hard-coding it here?
    auto group = EC_GROUP_Ptr(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
    if (ec_key.get() == nullptr || pkey.get() == nullptr) {
        LOG(ERROR) << "Memory allocation failed";
        return false;
    }

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        LOG(ERROR) << "Error generating key";
        return false;
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1) {
        LOG(ERROR) << "Error getting private key";
        return false;
    }

    const BIGNUM* bignum = EC_KEY_get0_private_key(ec_key.get());
    if (bignum == nullptr) {
        LOG(ERROR) << "Error getting bignum from private key";
        return false;
    }
    key.resize(BN_num_bytes(bignum));
    BN_bn2bin(bignum, key.data());

    ::keymaster::KeymasterKeyBlob key_material;
    if (::keymaster::EvpKeyToKeyMaterial(pkey.get(), &key_material) != KM_ERROR_OK) {
        LOG(ERROR) << "Error generating KM key blob";
        return false;
    }
    ::keymaster::AuthorizationSet authorizations = ::keymaster::AuthorizationSetBuilder()
                                                       .EcdsaSigningKey(224)
                                                       .Digest(KM_DIGEST_SHA_2_256)
                                                       .build();
    ::keymaster::AuthorizationSet hw_enforced;
    ::keymaster::AuthorizationSet sw_enforced;
    ::keymaster::PureSoftKeymasterContext context = ::keymaster::PureSoftKeymasterContext();
    ::keymaster::KeymasterKeyBlob key_blob;
    if (context.CreateKeyBlob(authorizations, KM_ORIGIN_GENERATED, key_material, &key_blob,
                              &hw_enforced, &sw_enforced) != KM_ERROR_OK) {
        LOG(ERROR) << "Error creating KM key blob";
        return false;
    }

    ::keymaster::UniquePtr<::keymaster::Key> keymasterKey;
    if (context.ParseKeyBlob(key_blob, authorizations, &keymasterKey) != KM_ERROR_OK) {
        LOG(ERROR) << "Error importing KM key blob";
        return false;
    }

    ::keymaster::AuthorizationSet attestationParams(
        ::keymaster::AuthorizationSetBuilder()
            .Authorization(::keymaster::TAG_ATTESTATION_CHALLENGE, "fake_challenge", 14)
            .Authorization(::keymaster::TAG_ATTESTATION_APPLICATION_ID, "fake_attest_app_id", 18));

    // TODO: need a way to set subject to "Android Identity Credential Key"
    // instead of "Android Keystore Key".
    ::keymaster::CertChainPtr cert_chain;
    if (context.GenerateAttestation(*(keymasterKey.get()), attestationParams, &cert_chain) !=
        KM_ERROR_OK) {
        LOG(ERROR) << "Error generating attestation";
        return false;
    }

    // Each entry in this certificate chain is a DER-encoded X.509
    // certificate. The chain starts with the certificate for |key|, followed by
    // N intermediate keys, following by the root key.
    certificate.resize(0);
    for (size_t n = 0; n < cert_chain->entry_count; n++) {
        keymaster_blob_t* blob = &(cert_chain->entries[n]);
        size_t certCurSize = certificate.size();
        certificate.resize(certCurSize + blob->data_length);
        memcpy(certificate.data() + certCurSize, blob->data, blob->data_length);
    }

    return true;
}

struct ECDSA_SIG_Deleter {
    void operator()(ECDSA_SIG* sig) const {
        if (sig != nullptr) {
            ECDSA_SIG_free(sig);
        }
    }
};

using ECDSA_SIG_Ptr = std::unique_ptr<ECDSA_SIG, ECDSA_SIG_Deleter>;

struct X509_Deleter {
    void operator()(X509* x509) const {
        if (x509 != nullptr) {
            X509_free(x509);
        }
    }
};

using X509_Ptr = std::unique_ptr<X509, X509_Deleter>;

static bool parseX509Certificates(const std::vector<uint8_t>& certificateChain,
                                  vector<X509_Ptr>& parsedCertificates) {
    const unsigned char* p = (unsigned char*)certificateChain.data();
    const unsigned char* pEnd = p + certificateChain.size();
    parsedCertificates.resize(0);
    while (p < pEnd) {
        auto x509 = X509_Ptr(d2i_X509(nullptr, &p, pEnd - p));
        if (x509 == nullptr) {
            LOG(ERROR) << "Error parsing X509 certificate";
            return false;
        }
        parsedCertificates.push_back(std::move(x509));
    }
    return true;
}

bool checkEcDsaSignature(const std::vector<uint8_t>& digest, const std::vector<uint8_t>& signature,
                         const std::vector<uint8_t>& certificateChain) {
    const unsigned char* p = (unsigned char*)signature.data();
    auto sig = ECDSA_SIG_Ptr(d2i_ECDSA_SIG(nullptr, &p, signature.size()));
    if (sig.get() == nullptr) {
        LOG(ERROR) << "Error decoding DER encoded signature";
        return false;
    }

    vector<X509_Ptr> certs;
    if (!parseX509Certificates(certificateChain, certs)) {
        return false;
    }
    if (certs.size() < 1) {
        LOG(ERROR) << "No certificates in chain";
        return false;
    }

    int algoId = OBJ_obj2nid(certs[0]->cert_info->key->algor->algorithm);
    if (algoId != NID_X9_62_id_ecPublicKey) {
        LOG(ERROR) << "Expected NID_ecEncryption, got " << OBJ_nid2ln(algoId);
        return false;
    }

    auto pkey = EVP_PKEY_Ptr(X509_get_pubkey(certs[0].get()));
    if (pkey.get() == nullptr) {
        LOG(ERROR) << "No public key";
        return false;
    }

    auto ecKey = EC_KEY_Ptr(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (ecKey.get() == nullptr) {
        LOG(ERROR) << "Failed getting EC key";
        return false;
    }

    int rc = ECDSA_do_verify(digest.data(), digest.size(), sig.get(), ecKey.get());
    if (rc != 1) {
        LOG(ERROR) << "Error verifying signature (rc=" << rc << ")";
        return false;
    }

    return true;
}

struct BIGNUM_Deleter {
    void operator()(BIGNUM* bignum) const {
        if (bignum != nullptr) {
            BN_free(bignum);
        }
    }
};

using BIGNUM_Ptr = std::unique_ptr<BIGNUM, BIGNUM_Deleter>;

vector<uint8_t> sha256(const vector<uint8_t>& data) {
    vector<uint8_t> ret;
    ret.resize(SHA256_DIGEST_LENGTH);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data.data(), data.size());
    SHA256_Final((unsigned char*)ret.data(), &ctx);
    return ret;
}

bool signEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data,
               vector<uint8_t>& signature) {
    auto bn = BIGNUM_Ptr(BN_bin2bn(key.data(), key.size(), nullptr));
    if (bn.get() == nullptr) {
        LOG(ERROR) << "Error creating BIGNUM";
        return false;
    }

    auto ec_key = EC_KEY_Ptr(EC_KEY_new_by_curve_name(NID_X9_62_prime256v1));
    if (EC_KEY_set_private_key(ec_key.get(), bn.get()) != 1) {
        LOG(ERROR) << "Error setting private key from BIGNUM";
        return false;
    }

    auto digest = sha256(data);
    ECDSA_SIG* sig = ECDSA_do_sign(digest.data(), digest.size(), ec_key.get());
    if (sig == nullptr) {
        LOG(ERROR) << "Error signing digest";
        return false;
    }
    size_t len = i2d_ECDSA_SIG(sig, nullptr);
    signature.resize(len);
    unsigned char* p = (unsigned char*)signature.data();
    i2d_ECDSA_SIG(sig, &p);
    ECDSA_SIG_free(sig);
    return true;
}

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

vector<EntryValue> entrySplitIntoChunks(const EntryValue& value, size_t chunkSize) {
    vector<EntryValue> ret;
    if (value.getDiscriminator() == EntryValue::hidl_discriminator::byteString) {
        if (value.byteString().size() > chunkSize) {
            size_t offset = 0;
            size_t remaining = value.byteString().size();
            const uint8_t* valueData = &(value.byteString()[0]);
            do {
                size_t size = std::min(remaining, chunkSize);
                vector<uint8_t> data;
                data.resize(size);
                memcpy(data.data(), valueData + offset, size);
                EntryValue value;
                value.byteString(data);
                ret.push_back(value);
                offset += size;
                remaining -= size;
            } while (remaining > 0);
            return ret;
        }
    } else if (value.getDiscriminator() == EntryValue::hidl_discriminator::textString) {
        if (value.textString().size() > chunkSize) {
            ret.push_back(value);
            return ret;
        }
    }
    ret.push_back(value);
    return ret;
}

vector<uint8_t> getTestHardwareBoundKey() {
    vector<uint8_t> HBK;
    HBK.resize(kAes128GcmKeySize);
    for (size_t n = 0; n < kAes128GcmKeySize; n++) {
        HBK[n] = 0;
    }
    return HBK;
}

bool entryCreateAdditionalData(const string& nameSpace, const string& name,
                               const vector<uint8_t> accessControlProfileIds,
                               vector<uint8_t>& encodedCbor) {
    cn_cbor_errback err;
    auto map = CnCborPtr(cn_cbor_map_create(&err));
    if (map.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringString(map.get(), "namespace", nameSpace.c_str())) {
        return false;
    }
    if (!cborMapPutStringString(map.get(), "name", name.c_str())) {
        return false;
    }
    cn_cbor* array = cn_cbor_array_create(&err);
    if (array == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating array (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringValue(map.get(), "accessControlProfileIds", array)) {
        return false;
    }
    for (uint8_t id : accessControlProfileIds) {
        if (!cborArrayAppendInt(array, id)) {
            return false;
        }
    }
    if (!cborEncode(map.get(), encodedCbor)) {
        return false;
    }
    return true;
}

bool AuthenticatedDataBuilder::reset(const std::string& docType,
                                     const std::vector<uint8_t>& encodedSessionTranscript) {
    docType_ = docType;
    encodedSessionTranscript_ = encodedSessionTranscript;

    return true;
}

bool AuthenticatedDataBuilder::addDataItem(const std::string& nameSpace, const std::string& key,
                                           const EntryValue& value) {
    items_.push_back(Item(nameSpace, key, value));
    return true;
}

static cn_cbor* EntryValueToCBor(const EntryValue& value) {
    cn_cbor* cbor_value;
    cn_cbor_errback err;
    switch (value.getDiscriminator()) {
        case EntryValue::hidl_discriminator::integer:
            cbor_value = cn_cbor_int_create(value.integer(), &err);
            break;
        case EntryValue::hidl_discriminator::textString:
            cbor_value = cn_cbor_string_create(value.textString().c_str(), &err);
            break;
        case EntryValue::hidl_discriminator::byteString:
            cbor_value =
                cn_cbor_data_create(value.byteString().data(), value.byteString().size(), &err);
            break;
        case EntryValue::hidl_discriminator::booleanValue:
            // There is no cn_cbor_bool_create()
            cbor_value = (cn_cbor*)calloc(1, sizeof(cn_cbor));
            cbor_value->type = value.booleanValue() ? CN_CBOR_TRUE : CN_CBOR_TRUE;
            break;
    }
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating object (pos " << err.pos << ")";
    }
    return cbor_value;
}

bool AuthenticatedDataBuilder::getEncodedCbor(std::vector<uint8_t>& encodedCbor) {
    cn_cbor_errback err;

    auto cbor = CnCborPtr(cn_cbor_map_create(&err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return false;
    }

    if (encodedSessionTranscript_.size() > 0) {
        cn_cbor* decodedSessionTranscript = cn_cbor_decode(encodedSessionTranscript_.data(),
                                                           encodedSessionTranscript_.size(), &err);
        if (decodedSessionTranscript == nullptr) {
            LOG(ERROR) << "Error " << err.err << " decoding encodedSessionTranscript (pos "
                       << err.pos << ")";
            return false;
        }
        if (!cborMapPutStringValue(cbor.get(), "SessionTranscript", decodedSessionTranscript)) {
            cn_cbor_free(decodedSessionTranscript);
            return false;
        }
    }

    cn_cbor* responseMap = cn_cbor_map_create(&err);
    if (responseMap == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating responseMap (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringValue(cbor.get(), "Response", responseMap)) {
        cn_cbor_free(responseMap);
        return false;
    }

    cn_cbor* docTypeMap = cn_cbor_map_create(&err);
    if (docTypeMap == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating docTypeMap (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringValue(responseMap, docType_.c_str(), docTypeMap)) {
        cn_cbor_free(docTypeMap);
        return false;
    }

    std::string curNameSpace;
    cn_cbor* curDataItemsMap = nullptr;
    for (const auto& item : items_) {
        if (curNameSpace != item.nameSpace_ || curDataItemsMap == nullptr) {
            curDataItemsMap = cn_cbor_map_create(&err);
            if (curDataItemsMap == nullptr) {
                LOG(ERROR) << "Error " << err.err << " creating curDataItemsMap (pos " << err.pos
                           << ")";
                return false;
            }
            if (!cborMapPutStringValue(docTypeMap, item.nameSpace_.c_str(), curDataItemsMap)) {
                cn_cbor_free(curDataItemsMap);
                return false;
            }
            curNameSpace = item.nameSpace_;
        }
        cn_cbor* cborValue = EntryValueToCBor(item.value_);
        if (cborValue == nullptr) {
            return false;
        }
        if (!cborMapPutStringValue(curDataItemsMap, item.name_.c_str(), cborValue)) {
            cn_cbor_free(cborValue);
            return false;
        }
    }

    if (!cborEncode(cbor.get(), encodedCbor)) {
        return false;
    }
    return true;
}

cn_cbor* SignedDataBuilder::Entry::toCbor() const {
    cn_cbor_errback err;

    cn_cbor* ret = cn_cbor_map_create(&err);
    if (ret == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return nullptr;
    }
    if (!cborMapPutStringString(ret, "name", name_.c_str())) {
        cn_cbor_free(ret);
        return nullptr;
    }
    cn_cbor* array = cn_cbor_array_create(&err);
    if (array == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating array (pos " << err.pos << ")";
        cn_cbor_free(ret);
        return nullptr;
    }
    if (!cborMapPutStringValue(ret, "accessControlProfiles", array)) {
        cn_cbor_free(array);
        cn_cbor_free(ret);
        return nullptr;
    }
    for (auto id : accessControlProfileIds_) {
        if (!cborArrayAppendInt(array, id)) {
            cn_cbor_free(ret);
            return nullptr;
        }
    }
    cn_cbor* cborValue = EntryValueToCBor(value_);
    if (cborValue == nullptr) {
        cn_cbor_free(ret);
        return nullptr;
    }
    if (!cborMapPutStringValue(ret, "value", cborValue)) {
        cn_cbor_free(cborValue);
        cn_cbor_free(ret);
        return nullptr;
    }
    if (!cborMapPutStringBool(ret, "directlyAvailable", directlyAvailable_)) {
        cn_cbor_free(ret);
        return nullptr;
    }

    return ret;
}

void SignedDataBuilder::reset(const std::string& docType, bool testCredential) {
    docType_ = docType;
    testCredential_ = testCredential;
    entries_.resize(0);
}

void SignedDataBuilder::addEntry(
    const std::string& nameSpace, const std::string& name,
    const std::vector<uint8_t>& accessControlProfileIds,
    const ::android::hardware::identity_credential::V1_0::EntryValue& value,
    bool directlyAvailable) {
    entries_.push_back(Entry(nameSpace, name, accessControlProfileIds, value, directlyAvailable));
}

void SignedDataBuilder::addAccessControlProfile(
    const SecureAccessControlProfile& accessControlProfile) {
    accessControlProfiles_.push_back(accessControlProfile);
}

bool SignedDataBuilder::getEncodedCbor(std::vector<uint8_t>& encodedCbor) {
    cn_cbor_errback err;

    auto cbor = CnCborPtr(cn_cbor_map_create(&err));
    if (cbor.get() == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
        return false;
    }

    if (!cborMapPutStringString(cbor.get(), "docType", docType_.c_str())) {
        return false;
    }

    cn_cbor* array = cn_cbor_array_create(&err);
    if (array == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating array (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringValue(cbor.get(), "accessControlProfiles", array)) {
        cn_cbor_free(array);
        return false;
    }
    for (const auto& profile : accessControlProfiles_) {
        cn_cbor* map = cn_cbor_map_create(&err);
        if (map == nullptr) {
            LOG(ERROR) << "Error " << err.err << " creating map (pos " << err.pos << ")";
            return false;
        }
        if (!cborArrayAppendValue(array, map)) {
            cn_cbor_free(map);
            return false;
        }
        if (!cborMapPutStringInt(map, "id", profile.id)) {
            return false;
        }
        if (profile.readerAuthPubKey.size() > 0) {
            if (!cborMapPutStringBStr(map, "readerAuthPubKey", profile.readerAuthPubKey.data(),
                                      profile.readerAuthPubKey.size())) {
                return false;
            }
        }
        if (profile.capabilityType !=
            ::android::hardware::keymaster::capability::V1_0::CapabilityType::NOT_APPLICABLE) {
            if (!cborMapPutStringInt(map, "capabilityType", int64_t(profile.capabilityType))) {
                return false;
            }
            if (profile.timeout > 0) {
                if (!cborMapPutStringInt(map, "timeout", int64_t(profile.timeout))) {
                    return false;
                }
            }
        }
    }

    cn_cbor* nameSpacesMap = cn_cbor_map_create(&err);
    if (nameSpacesMap == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating nameSpacesMap (pos " << err.pos << ")";
        return false;
    }
    if (!cborMapPutStringValue(cbor.get(), "namespaces", nameSpacesMap)) {
        cn_cbor_free(nameSpacesMap);
        return false;
    }

    std::string curNameSpace;
    cn_cbor* curEntriesArray = nullptr;
    for (const auto& entry : entries_) {
        if (curNameSpace != entry.nameSpace_ || curEntriesArray == nullptr) {
            curEntriesArray = cn_cbor_array_create(&err);
            if (curEntriesArray == nullptr) {
                LOG(ERROR) << "Error " << err.err << " creating curEntriesArray (pos " << err.pos
                           << ")";
                return false;
            }
            if (!cborMapPutStringValue(nameSpacesMap, entry.nameSpace_.c_str(), curEntriesArray)) {
                cn_cbor_free(curEntriesArray);
                return false;
            }
            curNameSpace = entry.nameSpace_;
        }
        cn_cbor* cborValue = entry.toCbor();
        if (cborValue == nullptr) {
            return false;
        }
        if (!cborArrayAppendValue(curEntriesArray, cborValue)) {
            cn_cbor_free(cborValue);
            return false;
        }
    }

    if (!cborMapPutStringBool(cbor.get(), "testCredential", testCredential_)) {
        return false;
    }

    if (!cborEncode(cbor.get(), encodedCbor)) {
        return false;
    }
    return true;
}

}  // namespace support
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
