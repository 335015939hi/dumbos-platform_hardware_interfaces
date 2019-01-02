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

#ifndef IDENTITY_CREDENTIAL_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
#define IDENTITY_CREDENTIAL_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_

#include <cstdint>
#include <vector>

#include <cn-cbor/cn-cbor.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace support {

struct CnCborDeleter {
    void operator()(cn_cbor* ptr) {
        if (ptr != nullptr) {
            cn_cbor_free(ptr);
        }
    }
};

typedef std::unique_ptr<cn_cbor, CnCborDeleter> CnCborPtr;

bool cborEncode(cn_cbor* value, std::vector<uint8_t>& encoded);
// Takes ownership of |cbor_value| on both success and error paths.
bool cborMapPutStringValue(cn_cbor* map, const std::string& key, cn_cbor* cbor_value);
bool cborMapPutStringString(cn_cbor* map, const std::string& key, const std::string& value);
bool cborMapPutStringBStr(cn_cbor* map, const std::string& key, const std::vector<uint8_t>& value);
bool cborMapPutStringInt(cn_cbor* map, const std::string& key, int64_t value);
// Takes ownership of |cbor_value| on both success and error paths.
bool cborArrayAppendValue(cn_cbor* array, cn_cbor* cbor_value);
bool cborArrayAppendInt(cn_cbor* array, int64_t value);

bool cborArrayGetBStr(cn_cbor* array, size_t idx, std::vector<uint8_t>& data);

bool entryCreateAdditionalData(const std::string& nameSpace, const std::string& name,
                               const std::vector<uint8_t> accessControlProfileIds,
                               std::vector<uint8_t>& encodedCbor);

std::vector<uint8_t> getTestHardwareBoundKey();

void hexdump(const std::string& name, const std::vector<uint8_t>& data);

bool getRandom(size_t numBytes, std::vector<uint8_t>& output);

// Decrypts |encryptedData| using |key| and |additionalAuthenticatedData|,
// writes resulting plaintext in |plainText|. The format of |encryptedData| must
// be as specified in the encryptAes128Gcm() function.
bool decryptAes128Gcm(const std::vector<uint8_t>& key, const std::vector<uint8_t>& encryptedData,
                      const std::vector<uint8_t>& additionalAuthenticatedData,
                      std::vector<uint8_t>& plainText);

// Encrypts |data| with |key| and |additionalAuthenticatedData| using |nonce|,
// writes resulting (nonce || ciphertext || tag) into |encryptedData|.
bool encryptAes128Gcm(const std::vector<uint8_t>& key, const std::vector<uint8_t>& nonce,
                      const std::vector<uint8_t>& data,
                      const std::vector<uint8_t>& additionalAuthenticatedData,
                      std::vector<uint8_t>& encryptedData);

}  // namespace support
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

#endif  // IDENTITY_CREDENTIAL_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
