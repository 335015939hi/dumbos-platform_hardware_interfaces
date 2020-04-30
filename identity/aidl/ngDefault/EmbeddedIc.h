/*
 * Copyright 2020, The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_IDENTITY_EMBEDDED_IC_H
#define ANDROID_HARDWARE_IDENTITY_EMBEDDED_IC_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// TODO: remove
#define EIC_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

// This header file contains platform specific details, including:
//
//   EIC_SHA256_CONTEXT_SIZE - the size of EicSha256Ctx
//
#include "EmbeddedIcOpsImpl.h"

#define EIC_SHA256_DIGEST_SIZE 32

// The size of a P-256 private key.
//
#define EIC_P256_PRIV_KEY_SIZE 32

// The size of a P-256 public key in uncompressed form.
//
// The public key is stored in uncompressed form, first the X coordinate, then
// the Y coordinate.
//
#define EIC_P256_PUB_KEY_SIZE 64

// The size of an ECSDA signature using P-256.
//
// The R and S values are stored here, first R then S.
//
#define EIC_ECDSA_P256_SIGNATURE_SIZE 64

#define EIC_AES_128_KEY_SIZE 16

// The following are definitions of implementation functions the
// underlying platform must provide.
//

struct EicSha256Ctx {
    uint8_t reserved[EIC_SHA256_CONTEXT_SIZE];
};
typedef struct EicSha256Ctx EicSha256Ctx;

#ifdef EIC_DEBUG
// Debug macro. Don't include a new-line in message.
//
// TODO: provide a way to turn this off and also a way to use something else
// than fprintf.
//
#define eicDebug(...)                            \
    do {                                         \
        eicPrint("%s:%d: ", __FILE__, __LINE__); \
        eicPrint(__VA_ARGS__);                   \
        eicPrint("\n");                          \
    } while (0)
#else
#define eicDebug(...) \
    do {              \
    } while (0)
#endif

// Prints message which should include new-line character. Can be no-op.
//
// Don't use this from code, use eicDebug() instead.
//
#ifdef EIC_DEBUG
void eicPrint(const char* format, ...);
#else
inline void eicPrint(const char* format, ...) {}
#endif

// Dumps data as pretty-printed hex. Can be no-op.
//
#ifdef EIC_DEBUG
void eicHexdump(const char* message, const uint8_t* data, size_t dataSize);
#else
inline void eicHexdump(const char* message, const uint8_t* data, size_t dataSize) {}
#endif

// Pretty-prints encoded CBOR. Can be no-op.
//
// If a byte-string is larger than |maxBStrSize| its contents will not be
// printed, instead the value of the form "<bstr size=1099016
// sha1=ef549cca331f73dfae2090e6a37c04c23f84b07b>" will be printed. Pass zero
// for |maxBStrSize| to disable this.
//
#ifdef EIC_DEBUG
void eicCborPrettyPrint(const uint8_t* cborData, size_t cborDataSize, size_t maxBStrSize);
#else
inline void eicCborPrettyPrint(const uint8_t* cborData, size_t cborDataSize, size_t maxBStrSize) {}
#endif

// Memory copying, see memcpy(3).
void* eicMemCpy(void* dest, const void* src, size_t n);

// String length, see strlen(3).
size_t eicStrLen(const char* s);

// Memory compare, see memcmp(3)
int eicMemCmp(const void* s1, const void* s2, size_t n);

// Random number generation.
bool eicOpsRandom(uint8_t* buf, size_t numBytes);

// If |testCredential| returns the 128-bit AES Hardware-Bound Key (16 bytes).
//
// Otherwise returns all zeroes (16 bytes).
//
const uint8_t* eicOpsGetHardwareBoundKey(bool testCredential);

// Encrypts |data| with |key| and |additionalAuthenticatedData| using |nonce|,
// returns the resulting (nonce || ciphertext || tag) in |encryptedData| which
// must be of size |dataSize| + 28.
bool eicOpsEncryptAes128Gcm(
        const uint8_t* key,    // Must be 16 bytes
        const uint8_t* nonce,  // Must be 12 bytes
        const uint8_t* data,   // May be NULL if size is 0
        size_t dataSize,
        const uint8_t* additionalAuthenticationData,  // May be NULL if size is 0
        size_t additionalAuthenticationDataSize, uint8_t* encryptedData);

// Decrypts |encryptedData| using |key| and |additionalAuthenticatedData|,
// returns resulting plaintext in |data| must be of size |encryptedDataSize| - 28.
//
// The format of |encryptedData| must be as specified in the
// encryptAes128Gcm() function.
bool eicOpsDecryptAes128Gcm(const uint8_t* key,  // Must be 16 bytes
                            const uint8_t* encryptedData, size_t encryptedDataSize,
                            const uint8_t* additionalAuthenticationData,
                            size_t additionalAuthenticationDataSize, uint8_t* data);

// Creates an EC key using the P-256 curve. The private key is written to
// |privateKey|. The public key is written to |publicKey|.
//
bool eicOpsCreateEcKey(uint8_t privateKey[EIC_P256_PRIV_KEY_SIZE],
                       uint8_t publicKey[EIC_P256_PUB_KEY_SIZE]);

// Generate an attestation certificate for the key identified by |publicKey|
// which must be of the form returned by eicOpsCreateEcKey().
//
// The certificate will be signed by the attestation keys the secure area has
// been provisioned with. The given |challenge| and |applicationId| will be
// used. (TODO: include other parameters needed as defined in the requirements
// in IWritableIdentityCredential::getAttestationCertificate().)
//
// The generated certificate will be in X.509 format and returned in |cert|
// and |certSize| must be set to the size of this array and this function will
// set it to the size of the certification chain on successfully return.
//
bool eicOpsAttestToEcKey(const uint8_t publicKey[EIC_P256_PUB_KEY_SIZE], const uint8_t* challenge,
                         size_t challengeSize, const uint8_t* applicationId,
                         size_t applicationIdSize, uint8_t* cert,
                         size_t* certSize);  // inout

// Returns an array which is EIC_P256_PUB_KEY_SIZE bytes long.
//
const uint8_t* eicOpsGetAttestationPublicKey(void);

// Returns the issuer name. This should be of the form
//
//  "credentialStoreName (credentialStoreAuthorName)"
//
// where credentialStoreName and credentialStoreAuthorName are the values the
// HAL will return in the IIdentityCredentialStore::getHardwareInformation()
// call.
//
// This string is used in the Issuer Name in X.509 certificates.
//
const char* eicOpsGetIssuerName(void);

// Generate an attestation certificate for the key identified by |publicKey|
// which must be of the form returned by eicOpsCreateEcKey().
//
// The certificate will be signed by the key identified by |signingKey| which
// must be of the form returned by eicOpsCreateEcKey().
//
// (TODO: define requirements about what goes in the certificate as defined by
// IIdentityCredential::generateSigningKeyPair().)
//
bool eicOpsSignEcKey(const uint8_t publicKey[EIC_P256_PUB_KEY_SIZE],
                     const uint8_t signingKey[EIC_P256_PRIV_KEY_SIZE], unsigned int serial,
                     const char* issuerName, const char* subjectName, time_t validityNotBefore,
                     time_t validityNotAfter, uint8_t* cert,
                     size_t* certSize);  // inout

// Uses |privateKey| to create an ECDSA signature of some data (the SHA-256 must
// be given by |digestOfData|). Returns the signature in |signature|.
//
bool eicOpsEcDsa(const uint8_t privateKey[EIC_P256_PRIV_KEY_SIZE],
                 const uint8_t digestOfData[EIC_SHA256_DIGEST_SIZE],
                 uint8_t signature[EIC_ECDSA_P256_SIGNATURE_SIZE]);

// SHA-256 functions.
void eicOpsSha256Init(EicSha256Ctx* ctx);
void eicOpsSha256Update(EicSha256Ctx* ctx, const uint8_t* data, size_t len);
void eicOpsSha256Final(EicSha256Ctx* ctx, uint8_t digest[EIC_SHA256_DIGEST_SIZE]);

/* --------------------------------------------------------------------------------------------- */

/* EicCbor is a utility class to build CBOR data structures and calculate
 * digests on the fly.
 */
typedef struct {
    // Contains the size of the built CBOR, even if it exceeds bufferSize (will
    // never write to buffer beyond bufferSize though)
    size_t size;

    // The size of the buffer. Is zero if no data is recorded in which case
    // only digesting is performed.
    size_t bufferSize;

    // The SHA-256 digester object.
    EicSha256Ctx digester;

    // The buffer used for building up CBOR or NULL if bufferSize is 0.
    uint8_t* buffer;
} EicCbor;

/* Initializes an EicCbor.
 *
 * The given buffer will be used, up to bufferSize.
 *
 * If bufferSize is 0, buffer may be NULL.
 */
void eicCborInit(EicCbor* cbor, uint8_t* buffer, size_t bufferSize);

/* Finishes building CBOR and returns the digest. */
void eicCborFinal(EicCbor* cbor, uint8_t digest[EIC_SHA256_DIGEST_SIZE]);

/* Appends CBOR data to the EicCbor. */
void eicCborAppend(EicCbor* cbor, const uint8_t* data, size_t size);

#define EIC_CBOR_MAJOR_TYPE_UNSIGNED 0
#define EIC_CBOR_MAJOR_TYPE_NEGATIVE 1
#define EIC_CBOR_MAJOR_TYPE_BYTE_STRING 2
#define EIC_CBOR_MAJOR_TYPE_STRING 3
#define EIC_CBOR_MAJOR_TYPE_ARRAY 4
#define EIC_CBOR_MAJOR_TYPE_MAP 5
#define EIC_CBOR_MAJOR_TYPE_SIMPLE 7

#define EIC_CBOR_SIMPLE_VALUE_FALSE 20
#define EIC_CBOR_SIMPLE_VALUE_TRUE 21

/* Begins a new CBOR value. */
void eicCborBegin(EicCbor* cbor, int majorType, size_t size);

/* Appends a bytestring. */
void eicCborAppendByteString(EicCbor* cbor, const uint8_t* data, size_t dataSize);

/* Appends a NUL-terminated UTF-8 string. */
void eicCborAppendString(EicCbor* cbor, const char* str);

/* Appends a simple value. */
void eicCborAppendSimple(EicCbor* cbor, uint8_t simpleValue);

/* Appends a boolean. */
void eicCborAppendBool(EicCbor* cbor, bool value);

/* Appends an unsigned number. */
void eicCborAppendUnsigned(EicCbor* cbor, uint64_t value);

/* Appends a number. */
void eicCborAppendNumber(EicCbor* cbor, int64_t value);

/* Starts appending an array.
 *
 * After this numElements CBOR elements must follow.
 */
void eicCborAppendArray(EicCbor* cbor, size_t numElements);

/* Starts appending a map.
 *
 * After this numPairs pairs of CBOR elements must follow.
 */
void eicCborAppendMap(EicCbor* cbor, size_t numPairs);

/* --------------------------------------------------------------------------------------------- */

#define EIC_MAX_NUM_NAMESPACES 32
#define EIC_MAX_NUM_ACCESS_CONTROL_PROFILE_IDS 32

typedef struct {
    // Set by eicCreateCredentialKey.
    uint8_t credentialPrivateKey[EIC_P256_PRIV_KEY_SIZE];

    int numEntryCounts;
    uint8_t entryCounts[EIC_MAX_NUM_NAMESPACES];

    int curNamespace;
    int curNamespaceNumProcessed;

    size_t curEntrySize;
    size_t curEntryNumBytesReceived;

    uint8_t storageKey[EIC_AES_128_KEY_SIZE];

    size_t expectedCborSizeAtEnd;

    // SHA-256 for AdditionalData, updated for each entry.
    uint8_t additionalDataSha256[EIC_SHA256_DIGEST_SIZE];

    EicCbor cbor;
} EicProvisioning;

bool eicProvisioningInit(EicProvisioning* ctx, bool testCredential);

bool eicProvisioningCreateCredentialKey(EicProvisioning* ctx, const uint8_t* challenge,
                                        size_t challengeSize, const uint8_t* applicationId,
                                        size_t applicationIdSize, uint8_t* publicKeyCert,
                                        size_t* publicKeyCertSize);

bool eicProvisioningStartPersonalization(EicProvisioning* ctx, int accessControlProfileCount,
                                         const int* entryCounts, size_t numEntryCounts,
                                         const char* docType,
                                         size_t expectedProofOfProvisioningingSize);

bool eicProvisioningAddAccessControlProfile(EicProvisioning* ctx, int id,
                                            const uint8_t* readerCertificate,
                                            size_t readerCertificateSize,
                                            bool userAuthenticationRequired, uint64_t timeoutMillis,
                                            uint64_t secureUserId, uint8_t outMac[28]);

bool eicProvisioningBeginAddEntry(EicProvisioning* ctx, const int* accessControlProfileIds,
                                  size_t numAccessControlProfileIds, const char* nameSpace,
                                  const char* name, uint64_t entrySize);

// The outEncryptedContent array must be contentSize + 28 bytes long.
bool eicProvisioningAddEntryValue(EicProvisioning* ctx, const int* accessControlProfileIds,
                                  size_t numAccessControlProfileIds, const char* nameSpace,
                                  const char* name, const uint8_t* content, size_t contentSize,
                                  uint8_t* outEncryptedContent);

// The data returned in |signatureOfToBeSigned| contains the ECDSA signature of
// the ToBeSigned CBOR from RFC 8051 "4.4. Signing and Verification Process"
// where content is set to the ProofOfProvisioninging CBOR.
//
bool eicProvisioningFinishAddingEntries(
        EicProvisioning* ctx, bool testCredential,
        uint8_t signatureOfToBeSigned[EIC_ECDSA_P256_SIGNATURE_SIZE]);

//
//
// The |encryptedCredentialKeys| array is set to AES-GCM-ENC(HBK, R, CredentialKeys, docType)
// where
//
//   CredentialKeys = [
//     bstr,   ; storageKey, a 128-bit AES key
//     bstr    ; credentialPrivKey, the private key for credentialKey
//   ]
//
// Since |storageKey| is 16 bytes and |credentialPrivKey| is 32 bytes, the
// encoded CBOR for CredentialKeys is 52 bytes and consequently
// |encryptedCredentialKeys| will be 52 + 28 = 80 bytes.
//
bool eicProvisioningFinishGetCredentialData(EicProvisioning* ctx, bool testCredential,
                                            const char* docType,
                                            uint8_t encryptedCredentialKeys[80]);

// ---------------------------------------------------------------------

typedef struct {
    uint8_t storageKey[EIC_AES_128_KEY_SIZE];
    uint8_t credentialPrivateKey[EIC_P256_PRIV_KEY_SIZE];

    uint8_t ephemeralPublicKey[EIC_P256_PUB_KEY_SIZE];

    uint64_t authChallenge;

    EicCbor cbor;
} EicPresentation;

bool eicPresentationInit(EicPresentation* ctx, bool testCredential, const char* docType,
                         const uint8_t encryptedCredentialKeys[80]);

bool eicPresentationGenerateSigningKeyPair(EicPresentation* ctx, const char* docType, time_t now,
                                           uint8_t* publicKeyCert, size_t* publicKeyCertSize,
                                           uint8_t signingKeyBlob[60]);

// Create an ephemeral key-pair.
//
// The public key is stored in |ctx->ephemeralPublicKey|, the private key is
// returned in |ephemeralPrivateKey|.
//
bool eicPresentationCreateEphemeralKeyPair(EicPresentation* ctx,
                                           uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE]);

bool eicPresentationCreateAuthChallenge(EicPresentation* ctx, uint64_t* authChallenge);

bool eicPresentationCheckHardwareAuthToken(EicPresentation* ctx, uint64_t challenge,
                                           uint64_t secureUserId, uint64_t authenticatorId,
                                           int hardwareAuthenticatorType, uint64_t timeStamp,
                                           const uint8_t* mac, size_t macSize);

// The reader could sign with a key that isn't P-256 so we can't assume the
// signature is EIC_ECDSA_P256_SIGNATURE_SIZE bytes long.
bool eicPresentationCheckReaderSignature(const uint8_t* itemsRequest, size_t itemsRequestSize,
                                         const uint8_t* sessionTranscript,
                                         size_t sessionTranscriptSize, int coseSignAlg,
                                         const uint8_t* readerSignatureOfToBeSigned,
                                         size_t readerSignatureOfToBeSignedSize,
                                         const uint8_t* readerCert, size_t readerCertSize);

bool eicPresentationCheckAccessControlProfile(EicPresentation* ctx, int id,
                                              const uint8_t* readerCertificate,
                                              size_t readerCertificateSize,
                                              bool userAuthenticationRequired, int timeoutMillis,
                                              uint64_t secureUserId, const uint8_t mac[28]);

// TODO: also convey + check other keys in the 'x5chain'

#ifdef __cplusplus
}
#endif

#endif  // ANDROID_HARDWARE_IDENTITY_EMBEDDED_IC_H
