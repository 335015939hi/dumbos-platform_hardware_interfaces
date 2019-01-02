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

using std::vector;

void hexdump(const std::string& name, const vector<uint8_t>& data) {
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

}  // namespace support
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android
