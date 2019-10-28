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

#ifndef IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
#define IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_

#include <cstdint>
#include <string>
#include <vector>

#include <android/hardware/identity/1.0/types.h>

namespace android {
namespace hardware {
namespace identity {
namespace support {

using ::std::string;
using ::std::vector;

using ::android::hardware::identity::V1_0::Result;
using ::android::hardware::identity::V1_0::ResultCode;
using ::android::hardware::identity::V1_0::SecureAccessControlProfile;

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
void hexdump(const string& name, const vector<uint8_t>& data);

string encodeHex(const string& str);

string encodeHex(const vector<uint8_t>& data);

string encodeHex(const uint8_t* data, size_t dataLen);

bool decodeHex(const string& hexEncoded, vector<uint8_t>& out);

// ---------------------------------------------------------------------------
// CBOR utilities.
// ---------------------------------------------------------------------------

// Returns pretty-printed CBOR for |value| in |out|.
//
// If a byte-string is larger than |maxBStrSize| its contents will not be
// printed, instead the value of the form "<bstr size=1099016
// sha1=ef549cca331f73dfae2090e6a37c04c23f84b07b>" will be printed. Pass zero
// for |maxBStrSize| to disable this.
//
// The |mapKeysToNotPrint| parameter specifies the name of map values
// to not print. This is useful for unit tests.
bool cborPrettyPrint(const vector<uint8_t>& encodedCbor, string& out, size_t maxBStrSize = 32,
                     const vector<string>& mapKeysToNotPrint = {});

// ---------------------------------------------------------------------------
// Crypto functionality / abstraction.
// ---------------------------------------------------------------------------

constexpr size_t kAesGcmIvSize = 12;
constexpr size_t kAesGcmTagSize = 16;
constexpr size_t kAes128GcmKeySize = 16;

// Writes |numBytes| bytes of random data to |output|.
bool getRandom(size_t numBytes, vector<uint8_t>& output);

// Calculates the SHA-256 of |data|.
vector<uint8_t> sha256(const vector<uint8_t>& data);

// Decrypts |encryptedData| using |key| and |additionalAuthenticatedData|,
// writes resulting plaintext in |plainText|. The format of |encryptedData| must
// be as specified in the encryptAes128Gcm() function.
bool decryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& encryptedData,
                      const vector<uint8_t>& additionalAuthenticatedData,
                      vector<uint8_t>& plainText);

// Encrypts |data| with |key| and |additionalAuthenticatedData| using |nonce|,
// writes resulting (nonce || ciphertext || tag) into |encryptedData|.
bool encryptAes128Gcm(const vector<uint8_t>& key, const vector<uint8_t>& nonce,
                      const vector<uint8_t>& data,
                      const vector<uint8_t>& additionalAuthenticatedData,
                      vector<uint8_t>& encryptedData);

// ---------------------------------------------------------------------------
// EC crypto functionality / abstraction (only supports P-256).
// ---------------------------------------------------------------------------

// Creates an 256-bit EC key using the NID_X9_62_prime256v1 curve, returns the
// PKCS#8 encoded key-pair in |keyPair|.
//
bool createEcKeyPair(vector<uint8_t>& keyPair);

// For an EC key |keyPair| encoded in PKCS#8 format, extracts the public key and
// stores it in |publicKey|, in uncompressed point form.
//
bool ecKeyPairGetPublicKey(const vector<uint8_t>& keyPair, vector<uint8_t>& publicKey);

// For an EC key |keyPair| encoded in PKCS#8 format, extracts the private key and
// stores it in |privateKey|, as an EC uncompressed key.
//
bool ecKeyPairGetPrivateKey(const vector<uint8_t>& keyPair, vector<uint8_t>& privateKey);

// For an EC key |keyPair| encoded in PKCS#8 format, creates a PKCS#12 structure
// with the key-pair (not using a password to encrypt the data). The public key
// in the created structure is included as a certificate, using the given fields
// |serialDecimal|, |issuer|, |subject|, |validityNotBefore|, and
// |validityNotAfter|.
//
bool ecKeyPairGetPkcs12(const vector<uint8_t>& keyPair, const string& name,
                        const string& serialDecimal, const string& issuer, const string& subject,
                        time_t validityNotBefore, time_t validityNotAfter,
                        vector<uint8_t>& pkcs12Bytes);

// Signs |data| with |key| (which must be in the format returned by
// ecKeyPairGetPrivateKey()). Signature is returned in |signature| and will be
// in DER format.
//
bool signEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data, vector<uint8_t>& signature);

// Calculates the HMAC with SHA-256 for |data| using |key|. The calculated HMAC
// is returned in |hmac| and will be 32 bytes.
//
bool hmacSha256(const vector<uint8_t>& key, const vector<uint8_t>& data, vector<uint8_t>& hmac);

// Checks that |signature| (in DER format) is a valid signature of |digest|,
// made with |publicKey| (which must be in the format returned by
// ecKeyPairGetPublicKey()).
//
bool checkEcDsaSignature(const vector<uint8_t>& digest, const vector<uint8_t>& signature,
                         const vector<uint8_t>& publicKey);

// Extracts the public-key from the top-most certificate in |certificateChain|
// (which should be a concatenated chain of DER-encoded X.509 certificates).
//
// The returned public key will be in the same format as returned by
// ecKeyPairGetPublicKey().
//
bool certificateChainGetTopMostKey(const vector<uint8_t>& certificateChain,
                                   vector<uint8_t>& publicKey);

// Generates a X.509 certificate for |publicKey| (which must be in the format
// returned by ecKeyPairGetPublicKey()).
//
// The certificate is signed by |signingKey| (which must be in the format
// returned by ecKeyPairGetPrivateKey())
//
bool ecPublicKeyGenerateCertificate(const vector<uint8_t>& publicKey,
                                    const vector<uint8_t>& signingKey, const string& serialDecimal,
                                    const string& issuer, const string& subject,
                                    time_t validityNotBefore, time_t validityNotAfter,
                                    vector<uint8_t>& certificate);

// Performs Elliptic-curve Diffie-Helman using |publicKey| (which must be in the
// format returned by ecKeyPairGetPublicKey()) and |privateKey| (which must be
// in the format returned by ecKeyPairGetPrivateKey()).
//
// The computed shared secret is returned in |sharedSecret|.
//
bool ecdh(const vector<uint8_t>& publicKey, const vector<uint8_t>& privateKey,
          vector<uint8_t>& sharedSecret);

// Key derivation function using SHA-256, conforming to RFC 5869.
//
// The derived key is returned in |derivedKey|.
//
bool hkdf(const vector<uint8_t>& sharedSecret, const vector<uint8_t>& salt,
          const vector<uint8_t>& info, size_t size, vector<uint8_t>& derivedKey);

// Returns the X and Y coordinates from |publicKey| (which must be in the format
// returned by ecKeyPairGetPublicKey()).
//
// The returned coordinates will be in uncompressed form.
//
bool ecPublicKeyGetXandY(const vector<uint8_t>& publicKey, vector<uint8_t>& x, vector<uint8_t>& y);

// Concatenates all certificates into |certificateChain| together into a
// single bytestring.
//
// This is the reverse operation of certificateChainSplit().
vector<uint8_t> certificateChainJoin(const vector<vector<uint8_t>>& certificateChain);

// Splits all the certificates in a single bytestring into individual
// certificates.
//
// Returns false if |certificateChain| contains invalid data.
//
// This is the reverse operation of certificateChainJoin().
bool certificateChainSplit(const vector<uint8_t>& certificateChain,
                           vector<vector<uint8_t>>& certificates);

// Validates that the certificate chain is valid. In particular, checks that each
// certificate in the chain is signed by the public key in the following certificate.
//
// Returns false if |certificateChain| failed validation or if each certificate
// is not signed by its successor.
//
bool certificateChainValidate(const vector<uint8_t>& certificateChain);

// Signs |data| and |detachedContent| with |key| (which must be in the format
// returned by ecKeyPairGetPrivateKey()).
//
// Signature is returned in |signatureCoseSign1| and will be in COSE_Sign1 format.
//
// If |certificateChain| is non-empty it's included in the 'x5chain'
// protected header element (as as described in'draft-ietf-cose-x509-04').
//
bool coseSignEcDsa(const vector<uint8_t>& key, const vector<uint8_t>& data,
                   const vector<uint8_t>& detachedContent, const vector<uint8_t>& certificateChain,
                   vector<uint8_t>& signatureCoseSign1);

// Checks that |signatureCoseSign1| (in COSE_Sign1 format) is a valid signature
// made with |public_key| (which must be in the format returned by
// ecKeyPairGetPublicKey()) where |detachedContent| is the detached content.
//
bool coseCheckEcDsaSignature(const vector<uint8_t>& signatureCoseSign1,
                             const vector<uint8_t>& detachedContent,
                             const vector<uint8_t>& publicKey);

// Copies the payload from a COSE_Sign1 to |data|.
bool coseSignGetPayload(const vector<uint8_t>& signatureCoseSign1, vector<uint8_t>& data);

// Extracts the X.509 certificate chain, if present. Returns the data as a
// concatenated chain of DER-encoded X.509 certificates
//
// Returns true if |certificateChain| was set, false if there is no
// 'x5chain' element or an error occurs.
//
bool coseSignGetX5Chain(const vector<uint8_t>& signatureCoseSign1,
                        vector<uint8_t>& certificateChain);

// MACs |data| and |detachedContent| with |key| (which can be any sequence of
// bytes).
//
// The MAC is returned in |coseMac0| and will be in COSE_Mac0 format.
//
bool coseMac0(const vector<uint8_t>& key, const vector<uint8_t>& data,
              const vector<uint8_t>& detachedContent, vector<uint8_t>& coseMac0);

// Creates a COSE_Key structure for the ECDSA key which must be in the format
// returned by ecKeyPairGetPublicKey().
bool coseCreateKey(const vector<uint8_t>& key, vector<uint8_t>& coseKey);

// ---------------------------------------------------------------------------
// Platform abstraction.
// ---------------------------------------------------------------------------

// Returns the hardware-bound AES-128 key.
const vector<uint8_t>& getHardwareBoundKey();

// ---------------------------------------------------------------------------
// Utility functions specific to IdentityCredential.
// ---------------------------------------------------------------------------

// Returns a reference to a Result with code OK and empty message.
const Result& resultOK();

// Returns a new Result with the given code and message.
Result result(ResultCode code, const char* format, ...) __attribute__((format(printf, 2, 3)));

// Splits the given bytestring into chunks. If the given vector is smaller or equal to
// |maxChunkSize| a vector with |content| as the only element is returned. Otherwise
// |content| is split into N vectors each of size |maxChunkSize| except the final element
// may be smaller than |maxChunkSize|.
vector<vector<uint8_t>> chunkVector(const vector<uint8_t>& content, size_t maxChunkSize);

// Constructs the CBOR representation of the data in |profile| used for
// calculating the MAC.
bool secureAccessControlProfileEncodeCbor(const SecureAccessControlProfile& profile,
                                          vector<uint8_t>& cborData);

// Calculates the MAC for |profile| using |storageKey|.
bool secureAccessControlProfileCalcMac(const SecureAccessControlProfile& profile,
                                       const vector<uint8_t>& storageKey, vector<uint8_t>& mac);

// Checks authenticity of the MAC in |profile| using |storageKey|.
bool secureAccessControlProfileCheckMac(const SecureAccessControlProfile& profile,
                                        const vector<uint8_t>& storageKey);

// Returns the testing AES-128 key where all bits are set to 0.
const vector<uint8_t>& getTestHardwareBoundKey();

// Creates the AdditionalData CBOR used in the addEntryValue() HIDL method.
bool entryCreateAdditionalData(const string& nameSpace, const string& name,
                               const vector<uint16_t> accessControlProfileIds,
                               vector<uint8_t>& encodedCbor);

}  // namespace support
}  // namespace identity
}  // namespace hardware
}  // namespace android

#endif  // IDENTITY_SUPPORT_INCLUDE_IDENTITY_CREDENTIAL_UTILS_H_
