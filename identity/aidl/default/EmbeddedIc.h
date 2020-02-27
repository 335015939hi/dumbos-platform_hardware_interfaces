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

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// This header file contains platform specific details, including:
//
//   EIC_SHA256_CONTEXT_SIZE - the size of EicSha256Ctx
//
#include "EmbeddedIcOpsImpl.h"

#define EIC_SHA256_DIGEST_SIZE 32

// The following are definitions of implementation functions the
// underlying platform must provide.
//

struct EicSha256Ctx {
    uint8_t reserved[EIC_SHA256_CONTEXT_SIZE];
};
typedef struct EicSha256Ctx EicSha256Ctx;

// Memory copying, see memcpy(3).
void* eicMemCpy(void* dest, const void* src, size_t n);

// String length, see strlen(3).
size_t eicStrLen(const char* s);

// Memory compare, see memcmp(3)
int eicMemCmp(const void* s1, const void* s2, size_t n);

// Random number generation.
bool eicOpsRandom(uint8_t* buf, size_t numBytes);

// Gets 128-bit AES Hardware-Bound Key (16 bytes)
const uint8_t* eicOpsGetHardwareBoundKey(void);

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

// Creates an 256-bit EC key using the NID_X9_62_prime256v1 curve, returns the
// PKCS#8 encoded key-pair in |keyPair|. The size of |keyPair| must be set in
// |keyPairSize| and on success this function will set it to the size of the
// generated key-pair.
//
// Also generates an attestation certificate chain using |challenge| and |applicationId|,
// and returns the generated certificate chain in X.509 format. This is returned in |certChain|
// and |certChainSize| must be set to the size of this array and this function will set it
// to the size of the certification chain on successfuly return.
//
// The attestation time fields used will be the current time, and expires in one year.
//
// (TODO: do we need to take the current time?)
//
bool eicOpsCreateEcKey(const uint8_t* challenge, size_t challengeSize, const uint8_t* applicationId,
                       size_t applicationIdSize, uint8_t* keyPair,
                       size_t* keyPairSize,  // inout
                       uint8_t* certChain,
                       size_t* certChainSize);  // inout

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

#define EIC_AES_128_KEY_SIZE 16

typedef struct {
    int numEntryCounts;
    uint8_t entryCounts[EIC_MAX_NUM_NAMESPACES];

    int curNamespace;
    int curNamespaceNumProcessed;

    size_t curEntrySize;
    size_t curEntryNumBytesReceived;

    uint8_t storageKey[EIC_AES_128_KEY_SIZE];

    // SHA-256 for AdditionalData, updated for each entry.
    uint8_t additionalDataSha256[EIC_SHA256_DIGEST_SIZE];

    EicCbor cbor;
} EicProvision;

bool eicProvisionInit(EicProvision* ctx, const char* docType, bool testCredential);

bool eicStartPersonalization(EicProvision* ctx, int accessControlProfileCount,
                             const int* entryCounts, size_t numEntryCounts);

bool eicAddAccessControlProfile(EicProvision* ctx, int id, const uint8_t* readerCertificate,
                                size_t readerCertificateSize, bool userAuthenticationRequired,
                                uint64_t timeoutMillis, uint64_t secureUserId, uint8_t outMac[28]);

bool eicBeginAddEntry(EicProvision* ctx, const int* accessControlProfileIds,
                      size_t numAccessControlProfileIds, const char* nameSpace, const char* name,
                      uint64_t entrySize);

// The outEncryptedContent array must be contentSize + 28 bytes long.
bool eicAddEntryValue(EicProvision* ctx, const int* accessControlProfileIds,
                      size_t numAccessControlProfileIds, const char* nameSpace, const char* name,
                      const uint8_t* content, size_t contentSize, uint8_t* outEncryptedContent);

bool eicFinishAddingEntries(EicProvision* ctx, bool testCredential,
                            uint8_t cborSha256[EIC_SHA256_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif

#endif  // ANDROID_HARDWARE_IDENTITY_EMBEDDED_IC_H
