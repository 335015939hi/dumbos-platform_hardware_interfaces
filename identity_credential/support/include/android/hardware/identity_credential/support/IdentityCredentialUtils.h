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
#include <string>
#include <vector>

#include <cn-cbor/cn-cbor.h>

#include <android/hardware/identity_credential/1.0/types.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace support {

using ::android::hardware::identity_credential::V1_0::EntryValue;
using ::android::hardware::identity_credential::V1_0::SecureAccessControlProfile;
using ::android::hardware::keymaster::capability::V1_0::KeymasterCapability;

// TODO: Right now this support library is a hodge-podge of utilities, software,
// and hardware abstraction. Since we only have a single implementation [1] this
// is fine ... but once we start adding other implementations, code should be
// refactored into separate libraries for easy code re-use.
//
// [1] : that is, the reference implementation in which runs entirely inside
//       Android and thus does not meet the security/privacy objectives of the
//       identity_credential HAL.

// ---------------------------------------------------------------------------
// Miscellaneous utilities.
// ---------------------------------------------------------------------------

// Dumps the data in |data| to stderr. The written data will be of the following
// form for the call hexdump("signature", data) where |data| is of size 71:
//
//   signature: dumping 71 bytes
//   0000  30 45 02 21 00 ac c6 12 60 56 a2 e9 ee 16 be 14  0E.!....`V......
//   0010  69 7f c4 00 95 8c e8 55 1f 22 de 34 0b 08 8a 3b  i......U.".4...;
//   0020  a0 56 54 05 07 02 20 58 77 d9 8c f9 eb 41 df fd  .VT... Xw....A..
//   0030  c1 a3 14 e0 bf b0 a2 c5 0c b6 85 8c 4a 0d f9 2b  ............J..+
//   0040  b7 8f d2 1d 9b 11 ac                             .......
//
// This should only be used for debugging.
void hexdump(const std::string& name, const std::vector<uint8_t>& data);

// ---------------------------------------------------------------------------
// CBOR / libcn-cbor utilities.
// ---------------------------------------------------------------------------

// Smart pointer for cn_cbor.
struct CnCborDeleter {
    void operator()(cn_cbor* ptr) {
        if (ptr != nullptr) {
            cn_cbor_free(ptr);
        }
    }
};
typedef std::unique_ptr<cn_cbor, CnCborDeleter> CnCborPtr;

// Encodes |value| in |encoded|
bool cborEncode(cn_cbor* value, std::vector<uint8_t>& encoded);

// Takes ownership of |cbor_value| on both success and error paths.
bool cborMapPutStringValue(cn_cbor* map, const char* key, cn_cbor* cbor_value);
bool cborMapPutStringString(cn_cbor* map, const char* key, const char* value);
bool cborMapPutStringBStr(cn_cbor* map, const char* key, const uint8_t* value, size_t value_size);
bool cborMapPutStringInt(cn_cbor* map, const char* key, int64_t value);
bool cborMapPutStringBool(cn_cbor* map, const char* key, bool value);
// Takes ownership of |cbor_value| on both success and error paths.
bool cborArrayAppendValue(cn_cbor* array, cn_cbor* cbor_value);
bool cborArrayAppendInt(cn_cbor* array, int64_t value);
bool cborArrayAppendBool(cn_cbor* array, bool value);
bool cborArrayAppendString(cn_cbor* array, const char* value);
bool cborArrayAppendBStr(cn_cbor* array, const uint8_t* value, size_t value_size);

bool cborArrayGetString(cn_cbor* array, size_t idx, std::string& data);
bool cborArrayGetBStr(cn_cbor* array, size_t idx, std::vector<uint8_t>& data);
bool cborArrayGetBool(cn_cbor* array, size_t idx, bool& data);

// Returns pretty-printed CBOR for |value| in |out|.
//
// If a byte-string is larger than |maxBStrSize| its contents will not be
// printed, instead the value of the form "<bstr size=1099016
// sha1=ef549cca331f73dfae2090e6a37c04c23f84b07b>" will be printed. Pass zero
// for |maxBStrSize| to disable this.
//
// The |mapKeysToNotPrint| parameter specifies the name of map values
// to not print. This is useful for unit tests.
bool cborPrettyPrint(cn_cbor* value, std::string& out, size_t maxBStrSize = 32,
                     const std::vector<std::string>& mapKeysToNotPrint = {});
bool cborPrettyPrint(const std::vector<uint8_t>& encodedCbor, std::string& out,
                     size_t maxBStrSize = 32,
                     const std::vector<std::string>& mapKeysToNotPrint = {});

// Checks if |value| has a byte-string with the given |valueBstr| anywhere in it.
bool cborHasBStr(cn_cbor* value, const std::vector<uint8_t>& valueBStr);

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

constexpr size_t kAesGcmIvSize = 12;
constexpr size_t kAesGcmTagSize = 16;
constexpr size_t kAes128GcmKeySize = 16;

// Writes |numBytes| bytes of random data to |output|.
bool getRandom(size_t numBytes, std::vector<uint8_t>& output);

// Calculates the SHA-256 of |data|.
std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);

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

// Generates a 256-bit EC key using the NID_X9_62_prime256v1 curve. Also
// generates an attestation certificate chain.
//
// The private key is returned in |key| as an EC uncompressed key and an X.509
// certificate signed by the Keymaster attestation certificate is returned in
// |certificate|.
bool createEcKeyAndAttestationChain(std::vector<uint8_t>& key, std::vector<uint8_t>& certificate);

// Signs |data| with |key| which must be in the format returned by
// createEcKeyAndAttestationChain(). Signature is returned in |signature|.
bool signEcDsa(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data,
               std::vector<uint8_t>& signature);

// Checks that |signature| (in DER format) is a valid signature of |digest|,
// made with the topmost public key of the certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
bool checkEcDsaSignature(const std::vector<uint8_t>& digest, const std::vector<uint8_t>& signature,
                         const std::vector<uint8_t>& certificateChain);

// Creates an EC key, returns the PKCS#8 encoded key-pair in |keyPair|.
bool createEcKeyPair(std::vector<uint8_t>& keyPair);

// For an EC key |keyPair| encoded in PKCS#8 format, extracts the public key and
// stores it in |publicKey|, in uncompressed point form.
bool ecKeyPairGetPublicKey(const std::vector<uint8_t>& keyPair, std::vector<uint8_t>& publicKey);

// For an EC key |keyPair| encoded in PKCS#8 format, extracts the private key and
// stores it in privateKey, as an EC uncompressed key.
bool ecKeyPairGetPrivateKey(const std::vector<uint8_t>& keyPair, std::vector<uint8_t>& privateKey);

// Generates a X.509 certificate for |publicKey| which must in uncompressed
// form. If |signingKey| is non-empty, it will be used to sign the certificate.
bool ecPublicKeyGenerateCertificate(const std::vector<uint8_t>& publicKey,
                                    const std::vector<uint8_t>& signingKey,
                                    const std::string& serialDecimal, const std::string& issuer,
                                    const std::string& subject, time_t validityNotBefore,
                                    time_t validityNotAfter, std::vector<uint8_t>& certificate);

// ---------------------------------------------------------------------------
// Platform abstraction.
// ---------------------------------------------------------------------------

bool validateAuthToken(KeymasterCapability authToken);

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

// Splits |value| into chunks so each chunk fits in |chunkSize|.
//
// For the case where |value| already fits, a vector of size 1 is returned.
std::vector<EntryValue> entrySplitIntoChunks(const EntryValue& value, size_t chunkSize);

// Returns the AES-128 key where all bits are set to 0.
std::vector<uint8_t> getTestHardwareBoundKey();

// Creates the AdditionalData CBOR used in the addEntryValue() HIDL method.
bool entryCreateAdditionalData(const std::string& nameSpace, const std::string& name,
                               const std::vector<uint8_t> accessControlProfileIds,
                               std::vector<uint8_t>& encodedCbor);

// Helper class for creating the AuthenticatedData CBOR returned in the
// finishRetrieval() HIDL method.
class AuthenticatedDataBuilder {
   public:
    AuthenticatedDataBuilder() {}

    bool reset(const std::string& docType, const std::vector<uint8_t>& encodedSessionTranscript);

    bool addDataItem(const std::string& nameSpace, const std::string& key,
                     const ::android::hardware::identity_credential::V1_0::EntryValue& value);

    bool getEncodedCbor(std::vector<uint8_t>& encodedCbor);

   private:
    struct Item {
        Item(const std::string& nameSpace, const std::string& name,
             const ::android::hardware::identity_credential::V1_0::EntryValue& value)
            : nameSpace_(nameSpace), name_(name), value_(value) {}
        std::string nameSpace_;
        std::string name_;
        ::android::hardware::identity_credential::V1_0::EntryValue value_;
    };

    std::string docType_;
    std::vector<uint8_t> encodedSessionTranscript_;
    std::vector<Item> items_;
};

// Helper class for creating the SignedData CBOR returned in the
// finishAddingEntries() HIDL method.
class SignedDataBuilder {
   public:
    SignedDataBuilder() {}

    void reset(const std::string& docType, bool testCredential);

    void addEntry(const std::string& nameSpace, const std::string& name,
                  const std::vector<uint8_t>& accessControlProfileIds,
                  const ::android::hardware::identity_credential::V1_0::EntryValue& value,
                  bool directlyAvailable);

    void addAccessControlProfile(const SecureAccessControlProfile& accessControlProfile);

    bool getEncodedCbor(std::vector<uint8_t>& encodedCbor);

   private:
    struct Entry {
        Entry() {}
        Entry(const std::string& nameSpace, const std::string& name,
              const std::vector<uint8_t>& accessControlProfileIds,
              const ::android::hardware::identity_credential::V1_0::EntryValue& value,
              bool directlyAvailable)
            : nameSpace_(nameSpace),
              name_(name),
              accessControlProfileIds_(accessControlProfileIds),
              value_(value),
              directlyAvailable_(directlyAvailable) {}

        cn_cbor* toCbor() const;

        std::string nameSpace_;
        std::string name_;
        std::vector<uint8_t> accessControlProfileIds_;
        ::android::hardware::identity_credential::V1_0::EntryValue value_;
        bool directlyAvailable_;
    };

    std::string docType_;
    bool testCredential_;
    std::vector<SecureAccessControlProfile> accessControlProfiles_;
    std::vector<Entry> entries_;
};

// TODO: this prototype is border unreadable/unusable - replace function with a Builder class.
bool generateRequestData(
    const std::vector<uint8_t>& encodedSessionTranscript,
    const std::vector<
        std::pair<std::string, std::vector<std::pair<std::string, std::vector<std::string>>>>>&
        docTypeAndNameSpaceAndDataItems,
    std::vector<uint8_t>& requestDataCbor);

}  // namespace support
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

#endif  // IDENTITY_CREDENTIAL_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
