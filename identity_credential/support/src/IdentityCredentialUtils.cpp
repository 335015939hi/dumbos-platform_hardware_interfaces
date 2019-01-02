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
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <android-base/logging.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace support {

using std::string;
using std::vector;

vector<uint8_t> getTestHardwareBoundKey() {
    vector<uint8_t> HBK;
    HBK.resize(16);
    for (size_t n = 0; n < 16; n++) {
        HBK[n] = 0;
    }
    return HBK;
}

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
    int cipherTextSize = int(encryptedData.size()) - 12 - 16;
    if (cipherTextSize < 0) {
        LOG(ERROR) << "encryptedData too small";
        return false;
    }
    unsigned char* nonce = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = nonce + 12;
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

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) {
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

    if (!EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, 16, tag)) {
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
    if (key.size() != 16) {
        LOG(ERROR) << "key is not 16 bytes";
        return false;
    }
    if (nonce.size() != 12) {
        LOG(ERROR) << "nonce is not 12 bytes";
        return false;
    }

    // The result is the nonce (12 bytes), the ciphertext, and finally the tag (16 bytes).
    encryptedData.resize(data.size() + 12 + 16);
    unsigned char* noncePtr = (unsigned char*)encryptedData.data();
    unsigned char* cipherText = noncePtr + 12;
    unsigned char* tag = cipherText + data.size();
    memcpy(noncePtr, nonce.data(), 12);

    auto ctx = EvpCipherCtxPtr(EVP_CIPHER_CTX_new());
    if (ctx.get() == nullptr) {
        LOG(ERROR) << "EVP_CIPHER_CTX_new: failed";
        return false;
    }

    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_128_gcm(), NULL, NULL, NULL) != 1) {
        LOG(ERROR) << "EVP_EncryptInit_ex: failed";
        return false;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, 12, NULL) != 1) {
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

    if (EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, 16, tag) != 1) {
        LOG(ERROR) << "EVP_CIPHER_CTX_ctrl: failed getting tag";
        return false;
    }

    return true;
}

bool cborEncode(cn_cbor* value, vector<uint8_t>& encoded) {
    // Unfortunately there's no way to know how big the encoded blob will be [1]
    // so we just hardcode a ceiling of 16 KiB for now... it's not very elegant
    // but it works.
    //
    // [1] : ideally cn_cbor_encoded_write(nullptr, 0, 0, array.get()) would
    // return how many bytes _would_ have been written just like sprintf() and
    // friends... but that's not how it works right now.
    encoded.resize(16384);
    ssize_t enc_sz = cn_cbor_encoder_write(encoded.data(), 0, encoded.size(), value);
    if (enc_sz == -1) {
        LOG(ERROR) << "Error encoding CBOR data";
        return false;
    }
    encoded.resize(enc_sz);
    return true;
}

// Takes ownership of |cbor_value| on both success and error paths.
bool cborMapPutStringValue(cn_cbor* map, const string& key, cn_cbor* cbor_value) {
    cn_cbor_errback err;

    if (!cn_cbor_mapput_string(map, key.c_str(), cbor_value, &err)) {
        LOG(ERROR) << "Error " << err.err << " putting value in map (pos " << err.pos << ")";
        cn_cbor_free(cbor_value);
        return false;
    }
    return true;
}

bool cborMapPutStringString(cn_cbor* map, const string& key, const string& value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_string_create(value.c_str(), &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating string (pos " << err.pos << ")";
        return false;
    }
    return cborMapPutStringValue(map, key, cbor_value);
}

bool cborMapPutStringBStr(cn_cbor* map, const string& key, const vector<uint8_t>& value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_data_create(value.data(), value.size(), &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating bstr (pos " << err.pos << ")";
        return false;
    }
    return cborMapPutStringValue(map, key, cbor_value);
}

bool cborMapPutStringInt(cn_cbor* map, const string& key, int64_t value) {
    cn_cbor_errback err;

    cn_cbor* cbor_value = cn_cbor_int_create(value, &err);
    if (cbor_value == nullptr) {
        LOG(ERROR) << "Error " << err.err << " creating int (pos " << err.pos << ")";
        return false;
    }
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

}  // namespace support
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
