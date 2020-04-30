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

#include "EmbeddedIc.h"

void eicCborInit(EicCbor* cbor, uint8_t* buffer, size_t bufferSize) {
    cbor->size = 0;
    cbor->bufferSize = bufferSize;
    cbor->buffer = buffer;
    eicOpsSha256Init(&cbor->digester);
}

void eicCborFinal(EicCbor* cbor, uint8_t digest[EIC_SHA256_DIGEST_SIZE]) {
    eicOpsSha256Final(&cbor->digester, digest);
}

void eicCborAppend(EicCbor* cbor, const uint8_t* data, size_t size) {
    eicOpsSha256Update(&cbor->digester, data, size);

    if (cbor->size >= cbor->bufferSize) {
        cbor->size += size;
        return;
    }

    size_t numBytesLeft = cbor->bufferSize - cbor->size;
    size_t numBytesToCopy = size;
    if (numBytesToCopy > numBytesLeft) {
        numBytesToCopy = numBytesLeft;
    }
    eicMemCpy(cbor->buffer + cbor->size, data, numBytesToCopy);

    cbor->size += size;
}

void eicCborBegin(EicCbor* cbor, int majorType, size_t size) {
    uint8_t data[9];

    if (size < 24) {
        data[0] = (majorType << 5) | size;
        eicCborAppend(cbor, data, 1);
    } else if (size <= 0xff) {
        data[0] = (majorType << 5) | 24;
        data[1] = size;
        eicCborAppend(cbor, data, 2);
    } else if (size <= 0xffff) {
        data[0] = (majorType << 5) | 25;
        data[1] = size >> 8;
        data[2] = size & 0xff;
        eicCborAppend(cbor, data, 3);
    } else if (size <= 0xffffffff) {
        data[0] = (majorType << 5) | 26;
        data[1] = (size >> 24) & 0xff;
        data[2] = (size >> 16) & 0xff;
        data[3] = (size >> 8) & 0xff;
        data[4] = size & 0xff;
        eicCborAppend(cbor, data, 5);
    } else {
        data[0] = (majorType << 5) | 24;
        data[1] = (size >> 56) & 0xff;
        data[2] = (size >> 48) & 0xff;
        data[3] = (size >> 40) & 0xff;
        data[4] = (size >> 32) & 0xff;
        data[5] = (size >> 24) & 0xff;
        data[6] = (size >> 16) & 0xff;
        data[7] = (size >> 8) & 0xff;
        data[8] = size & 0xff;
        eicCborAppend(cbor, data, 9);
    }
}

void eicCborAppendByteString(EicCbor* cbor, const uint8_t* data, size_t dataSize) {
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_BYTE_STRING, dataSize);
    eicCborAppend(cbor, data, dataSize);
}

void eicCborAppendString(EicCbor* cbor, const char* str) {
    size_t length = eicStrLen(str);
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_STRING, length);
    eicCborAppend(cbor, (const uint8_t*)str, length);
}

void eicCborAppendSimple(EicCbor* cbor, uint8_t simpleValue) {
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_SIMPLE, simpleValue);
}

void eicCborAppendBool(EicCbor* cbor, bool value) {
    uint8_t simpleValue = value ? EIC_CBOR_SIMPLE_VALUE_TRUE : EIC_CBOR_SIMPLE_VALUE_FALSE;
    eicCborAppendSimple(cbor, simpleValue);
}

void eicCborAppendUnsigned(EicCbor* cbor, uint64_t value) {
    size_t encoded = value;
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_UNSIGNED, encoded);
}

void eicCborAppendNumber(EicCbor* cbor, int64_t value) {
    if (value < 0) {
        size_t encoded = -1 - value;
        eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_NEGATIVE, encoded);
    } else {
        eicCborAppendUnsigned(cbor, value);
    }
}

void eicCborAppendArray(EicCbor* cbor, size_t numElements) {
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_ARRAY, numElements);
}

void eicCborAppendMap(EicCbor* cbor, size_t numPairs) {
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_MAP, numPairs);
}

// static uint8_t scratch[1024*1024];

bool eicProvisioningInit(EicProvisioning* ctx, bool testCredential) {
    eicMemSet(ctx, '\0', sizeof(EicProvisioning));
    if (!eicOpsRandom(ctx->storageKey, EIC_AES_128_KEY_SIZE)) {
        return false;
    }

    return true;
}

bool eicProvisioningCreateCredentialKey(EicProvisioning* ctx, const uint8_t* challenge,
                                        size_t challengeSize, const uint8_t* applicationId,
                                        size_t applicationIdSize, uint8_t* publicKeyCert,
                                        size_t* publicKeyCertSize) {
    uint8_t publicKey[EIC_P256_PUB_KEY_SIZE];

    if (!eicOpsCreateEcKey(ctx->credentialPrivateKey, publicKey)) {
        return false;
    }
    if (!eicOpsAttestToEcKey(publicKey, challenge, challengeSize, applicationId, applicationIdSize,
                             publicKeyCert, publicKeyCertSize)) {
        return false;
    }
    return true;
}

#ifdef EIC_DEBUG
static uint8_t eicProvisioningStaticBuf[1024 * 1024] = {0};
#endif

bool eicProvisioningStartPersonalization(EicProvisioning* ctx, int accessControlProfileCount,
                                         const int* entryCounts, size_t numEntryCounts,
                                         const char* docType,
                                         size_t expectedProofOfProvisioningSize) {
    if (numEntryCounts >= EIC_MAX_NUM_NAMESPACES) {
        return false;
    }
    if (accessControlProfileCount >= EIC_MAX_NUM_ACCESS_CONTROL_PROFILE_IDS) {
        return false;
    }

    ctx->numEntryCounts = numEntryCounts;
    if (numEntryCounts > EIC_MAX_NUM_NAMESPACES) {
        return false;
    }
    for (size_t n = 0; n < numEntryCounts; n++) {
        if (entryCounts[n] >= 256) {
            return false;
        }
        ctx->entryCounts[n] = entryCounts[n];
    }
    ctx->curNamespace = -1;
    ctx->curNamespaceNumProcessed = 0;

#ifdef EIC_DEBUG
    eicCborInit(&ctx->cbor, eicProvisioningStaticBuf, sizeof(eicProvisioningStaticBuf));
#else
    eicCborInit(&ctx->cbor, NULL, 0);
#endif

    // What we're going to sign is the COSE ToBeSigned structure which
    // looks like the following:
    //
    // Sig_structure = [
    //   context : "Signature" / "Signature1" / "CounterSignature",
    //   body_protected : empty_or_serialized_map,
    //   ? sign_protected : empty_or_serialized_map,
    //   external_aad : bstr,
    //   payload : bstr
    //  ]
    //
    eicCborAppendArray(&ctx->cbor, 4);
    eicCborAppendString(&ctx->cbor, "Signature1");

    // The COSE Encoded protected headers is just a single field with
    // COSE_LABEL_ALG (1) -> COSE_ALG_ECSDA_256 (-7). For simplicitly we just
    // hard-code the CBOR encoding:
    static const uint8_t coseEncodedProtectedHeaders[] = {0xa1, 0x01, 0x26};
    eicCborAppendByteString(&ctx->cbor, coseEncodedProtectedHeaders,
                            sizeof(coseEncodedProtectedHeaders));

    // We currently don't support Externally Supplied Data (RFC 8152 section 4.3)
    // so external_aad is the empty bstr
    static const uint8_t externalAad[0] = {};
    eicCborAppendByteString(&ctx->cbor, externalAad, sizeof(externalAad));

    // For the payload, the _encoded_ form follows here. We handle this by simply
    // opening a bstr, and then writing the CBOR. This requires us to known the
    // size of said bstr, ahead of time.
    eicCborBegin(&ctx->cbor, EIC_CBOR_MAJOR_TYPE_BYTE_STRING, expectedProofOfProvisioningSize);
    ctx->expectedCborSizeAtEnd = expectedProofOfProvisioningSize + ctx->cbor.size;

    eicCborAppendArray(&ctx->cbor, 5);
    eicCborAppendString(&ctx->cbor, "ProofOfProvisioning");
    eicCborAppendString(&ctx->cbor, docType);

    eicCborAppendArray(&ctx->cbor, accessControlProfileCount);

    return true;
}

static bool eicCalcAccessControlCbor(EicCbor* cborBuilder, int id, const uint8_t* readerCertificate,
                                     size_t readerCertificateSize, bool userAuthenticationRequired,
                                     uint64_t timeoutMillis, uint64_t secureUserId) {
    if (userAuthenticationRequired) {
        if (readerCertificateSize > 0) {
            eicCborAppendMap(cborBuilder, 4);
            eicCborAppendString(cborBuilder, "id");
            eicCborAppendUnsigned(cborBuilder, id);
            eicCborAppendString(cborBuilder, "readerCertificate");
            eicCborAppendByteString(cborBuilder, readerCertificate, readerCertificateSize);
            eicCborAppendString(cborBuilder, "userAuthenticationRequired");
            eicCborAppendBool(cborBuilder, userAuthenticationRequired);
            eicCborAppendString(cborBuilder, "timeoutMillis");
            eicCborAppendUnsigned(cborBuilder, timeoutMillis);
        } else {
            eicCborAppendMap(cborBuilder, 3);
            eicCborAppendString(cborBuilder, "id");
            eicCborAppendUnsigned(cborBuilder, id);
            eicCborAppendString(cborBuilder, "userAuthenticationRequired");
            eicCborAppendBool(cborBuilder, userAuthenticationRequired);
            eicCborAppendString(cborBuilder, "timeoutMillis");
            eicCborAppendUnsigned(cborBuilder, timeoutMillis);
        }
    } else {
        if (readerCertificateSize > 0) {
            eicCborAppendMap(cborBuilder, 2);
            eicCborAppendString(cborBuilder, "id");
            eicCborAppendUnsigned(cborBuilder, id);
            eicCborAppendString(cborBuilder, "readerCertificate");
            eicCborAppendByteString(cborBuilder, readerCertificate, readerCertificateSize);
        } else {
            eicCborAppendMap(cborBuilder, 1);
            eicCborAppendString(cborBuilder, "id");
            eicCborAppendUnsigned(cborBuilder, id);
        }
    }

    if (cborBuilder->size > cborBuilder->bufferSize) {
        eicDebug("Buffer for ACP CBOR is too small (%zd) - need %zd bytes", cborBuilder->bufferSize,
                 cborBuilder->size);
        return false;
    }

    return true;
}

// Ugh, the CBOR can get pretty big because it contains the reader certificate...
//
// TODO: this means currently allocating 512 bytes on the stack which may not
//       work well.. look into incrementally doing this or use scratch space
//       in e.g. the output buffer.
//
#define EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE 512

bool eicProvisioningAddAccessControlProfile(EicProvisioning* ctx, int id,
                                            const uint8_t* readerCertificate,
                                            size_t readerCertificateSize,
                                            bool userAuthenticationRequired, uint64_t timeoutMillis,
                                            uint64_t secureUserId, uint8_t outMac[28]) {
    uint8_t cborBuffer[EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE];
    EicCbor cborBuilder;

    eicCborInit(&cborBuilder, cborBuffer, EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE);

    if (!eicCalcAccessControlCbor(&cborBuilder, id, readerCertificate, readerCertificateSize,
                                  userAuthenticationRequired, timeoutMillis, secureUserId)) {
        return false;
    }

    // Calculate and return MAC
    uint8_t nonce[12];
    if (!eicOpsRandom(nonce, 12)) {
        return false;
    }
    if (!eicOpsEncryptAes128Gcm(ctx->storageKey, nonce, NULL, 0, cborBuilder.buffer,
                                cborBuilder.size, outMac)) {
        return false;
    }

    // Append the CBOR from the local builder to the digester.
    eicCborAppend(&ctx->cbor, cborBuilder.buffer, cborBuilder.size);

    return true;
}

static bool calcAdditionalData(const int* accessControlProfileIds,
                               size_t numAccessControlProfileIds, const char* nameSpace,
                               const char* name, uint8_t* cborBuffer, size_t cborBufferSize,
                               size_t* outAdditionalDataCborSize,
                               uint8_t additionalDataSha256[EIC_SHA256_DIGEST_SIZE]) {
    EicCbor cborBuilder;

    eicCborInit(&cborBuilder, cborBuffer, cborBufferSize);
    eicCborAppendMap(&cborBuilder, 3);
    eicCborAppendString(&cborBuilder, "Namespace");
    eicCborAppendString(&cborBuilder, nameSpace);
    eicCborAppendString(&cborBuilder, "Name");
    eicCborAppendString(&cborBuilder, name);
    eicCborAppendString(&cborBuilder, "AccessControlProfileIds");
    eicCborAppendArray(&cborBuilder, numAccessControlProfileIds);
    for (size_t n = 0; n < numAccessControlProfileIds; n++) {
        eicCborAppendNumber(&cborBuilder, accessControlProfileIds[n]);
    }
    if (cborBuilder.size > cborBufferSize) {
        eicDebug("Not enough space for additionalData - buffer is only %zd bytes, content is %zd",
                 cborBufferSize, cborBuilder.size);
        return false;
    }
    if (outAdditionalDataCborSize != NULL) {
        *outAdditionalDataCborSize = cborBuilder.size;
    }
    eicCborFinal(&cborBuilder, additionalDataSha256);
    return true;
}

bool eicProvisioningBeginAddEntry(EicProvisioning* ctx, const int* accessControlProfileIds,
                                  size_t numAccessControlProfileIds, const char* nameSpace,
                                  const char* name, uint64_t entrySize) {
    fprintf(stderr, "cn=%d beginAddEntry, ns %s name %s\n", ctx->curNamespace, nameSpace, name);

    uint8_t additionalDataCbor[512];  // TODO: pick a size that's big enough
    size_t additionalDataCborSize;

    // We'll need to calc and store a digest of additionalData to check that it's the same
    // additionalData being passed in for every eicProvisioningAddEntryValue() call...
    if (!calcAdditionalData(accessControlProfileIds, numAccessControlProfileIds, nameSpace, name,
                            additionalDataCbor, sizeof additionalDataCbor, &additionalDataCborSize,
                            ctx->additionalDataSha256)) {
        return false;
    }

    if (ctx->curNamespace == -1) {
        ctx->curNamespace = 0;
        ctx->curNamespaceNumProcessed = 0;
        // Opens the main map: { * Namespace => [ + Entry ] }
        eicCborAppendMap(&ctx->cbor, ctx->numEntryCounts);
        eicCborAppendString(&ctx->cbor, nameSpace);
        // Opens the per-namespace array: [ + Entry ]
        eicCborAppendArray(&ctx->cbor, ctx->entryCounts[ctx->curNamespace]);
    }

    if (ctx->curNamespaceNumProcessed == ctx->entryCounts[ctx->curNamespace]) {
        ctx->curNamespace += 1;
        ctx->curNamespaceNumProcessed = 0;
        eicCborAppendString(&ctx->cbor, nameSpace);
        // Opens the per-namespace array: [ + Entry ]
        eicCborAppendArray(&ctx->cbor, ctx->entryCounts[ctx->curNamespace]);
    }

    eicCborAppendMap(&ctx->cbor, 3);
    eicCborAppendString(&ctx->cbor, "name");
    eicCborAppendString(&ctx->cbor, name);

    ctx->curEntrySize = entrySize;
    ctx->curEntryNumBytesReceived = 0;

    eicCborAppendString(&ctx->cbor, "value");

    ctx->curNamespaceNumProcessed += 1;
    return true;
}

bool eicProvisioningAddEntryValue(EicProvisioning* ctx, const int* accessControlProfileIds,
                                  size_t numAccessControlProfileIds, const char* nameSpace,
                                  const char* name, const uint8_t* content, size_t contentSize,
                                  uint8_t* outEncryptedContent) {
    uint8_t additionalDataCbor[512];  // TODO: pick a size that's big enough
    size_t additionalDataCborSize;

    uint8_t calculatedSha256[EIC_SHA256_DIGEST_SIZE];
    if (!calcAdditionalData(accessControlProfileIds, numAccessControlProfileIds, nameSpace, name,
                            additionalDataCbor, sizeof additionalDataCbor, &additionalDataCborSize,
                            calculatedSha256)) {
        return false;
    }
    if (eicMemCmp(calculatedSha256, ctx->additionalDataSha256, EIC_SHA256_DIGEST_SIZE) != 0) {
        eicDebug("SHA-256 mismatch of additionalData");
        return false;
    }

    eicCborAppend(&ctx->cbor, content, contentSize);

    uint8_t nonce[12];
    if (!eicOpsRandom(nonce, 12)) {
        return false;
    }
    if (!eicOpsEncryptAes128Gcm(ctx->storageKey, nonce, content, contentSize, additionalDataCbor,
                                additionalDataCborSize, outEncryptedContent)) {
        return false;
    }

    // If done with this entry, close the map
    ctx->curEntryNumBytesReceived += contentSize;
    if (ctx->curEntryNumBytesReceived == ctx->curEntrySize) {
        eicCborAppendString(&ctx->cbor, "accessControlProfiles");
        eicCborAppendArray(&ctx->cbor, numAccessControlProfileIds);
        for (size_t n = 0; n < numAccessControlProfileIds; n++) {
            eicCborAppendNumber(&ctx->cbor, accessControlProfileIds[n]);
        }
    }
    return true;
}

bool eicProvisioningFinishAddingEntries(
        EicProvisioning* ctx, bool testCredential,
        uint8_t signatureOfToBeSigned[EIC_ECDSA_P256_SIGNATURE_SIZE]) {
    uint8_t cborSha256[EIC_SHA256_DIGEST_SIZE];

    eicCborAppendBool(&ctx->cbor, testCredential);
    eicCborFinal(&ctx->cbor, cborSha256);

    // This verifies that the correct expectedProofOfProvisioningSize value was
    // passed in at eicStartPersonalization() time.
    if (ctx->cbor.size != ctx->expectedCborSizeAtEnd) {
        eicDebug("CBOR size is %zd, was expecting %zd", ctx->cbor.size, ctx->expectedCborSizeAtEnd);
        return false;
    }

    if (!eicOpsEcDsa(ctx->credentialPrivateKey, cborSha256, signatureOfToBeSigned)) {
        eicDebug("Error signing proofOfProvisioning");
        return false;
    }

    return true;
}

bool eicProvisioningFinishGetCredentialData(EicProvisioning* ctx, bool testCredential,
                                            const char* docType,
                                            uint8_t encryptedCredentialKeys[80]) {
    EicCbor cbor;
    uint8_t cborBuf[52];

    eicCborInit(&cbor, cborBuf, sizeof(cborBuf));
    eicCborAppendArray(&cbor, 2);
    eicCborAppendByteString(&cbor, ctx->storageKey, EIC_AES_128_KEY_SIZE);
    eicCborAppendByteString(&cbor, ctx->credentialPrivateKey, EIC_P256_PRIV_KEY_SIZE);
    if (cbor.size > sizeof(cborBuf)) {
        eicDebug("Exceeded buffer size");
        return false;
    }

    uint8_t nonce[12];
    if (!eicOpsRandom(nonce, 12)) {
        eicDebug("Error getting random");
        return false;
    }
    if (!eicOpsEncryptAes128Gcm(
                eicOpsGetHardwareBoundKey(testCredential), nonce, cborBuf, cbor.size,
                // DocType is the additionalAuthenticatedData
                (const uint8_t*)docType, eicStrLen(docType), encryptedCredentialKeys)) {
        eicDebug("Error encrypting CredentialKeys");
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------

bool eicPresentationInit(EicPresentation* ctx, bool testCredential, const char* docType,
                         const uint8_t encryptedCredentialKeys[80]) {
    uint8_t credentialKeys[52];

    eicMemSet(ctx, '\0', sizeof(EicPresentation));

    if (!eicOpsDecryptAes128Gcm(eicOpsGetHardwareBoundKey(testCredential), encryptedCredentialKeys,
                                80,
                                // DocType is the additionalAuthenticatedData
                                (const uint8_t*)docType, eicStrLen(docType), credentialKeys)) {
        eicDebug("Error decrypting CredentialKeys");
        return false;
    }

    // It's supposed to look like this;
    //
    //         CredentialKeys = [
    //              bstr,   ; storageKey, a 128-bit AES key
    //              bstr    ; credentialPrivKey, the private key for credentialKey
    //         ]
    //
    // where storageKey is 16 bytes and credentialPrivateKey is 32 bytes.
    //
    // So the first two bytes will be 0x82 0x50 indicating resp. an array of two elements
    // and a bstr of 16 elements. Sixteen bytes later (offset 18 and 19) there will be
    // a bstr of 32 bytes. It's encoded as two bytes 0x58 and 0x20.
    //
    if (credentialKeys[0] != 0x82 || credentialKeys[1] != 0x50 || credentialKeys[18] != 0x58 ||
        credentialKeys[19] != 0x20) {
        eicDebug("Invalid CBOR for CredentialKeys");
        return false;
    }
    eicMemCpy(ctx->storageKey, credentialKeys + 2, EIC_AES_128_KEY_SIZE);
    eicMemCpy(ctx->credentialPrivateKey, credentialKeys + 20, EIC_P256_PRIV_KEY_SIZE);
    return true;
}

bool eicPresentationGenerateSigningKeyPair(EicPresentation* ctx, const char* docType, time_t now,
                                           uint8_t* publicKeyCert, size_t* publicKeyCertSize,
                                           uint8_t signingKeyBlob[60]) {
    uint8_t signingKeyPriv[EIC_P256_PRIV_KEY_SIZE];
    uint8_t signingKeyPub[EIC_P256_PUB_KEY_SIZE];

    if (!eicOpsCreateEcKey(signingKeyPriv, signingKeyPub)) {
        eicDebug("Error creating signing key");
        return false;
    }

    const int secondsInOneYear = 365 * 24 * 60 * 60;
    time_t validityNotBefore = now;
    time_t validityNotAfter = now + secondsInOneYear;  // One year from now.
    if (!eicOpsSignEcKey(signingKeyPub, ctx->credentialPrivateKey, 1,
                         "Android Identity Credential Authentication Key", eicOpsGetIssuerName(),
                         validityNotBefore, validityNotAfter, publicKeyCert, publicKeyCertSize)) {
        eicDebug("Error creating certificate for signing key");
        return false;
    }

    uint8_t nonce[12];
    if (!eicOpsRandom(nonce, 12)) {
        eicDebug("Error getting random");
        return false;
    }
    if (!eicOpsEncryptAes128Gcm(ctx->storageKey, nonce, signingKeyPriv, sizeof(signingKeyPriv),
                                // DocType is the additionalAuthenticatedData
                                (const uint8_t*)docType, eicStrLen(docType), signingKeyBlob)) {
        eicDebug("Error encrypting signing key");
        return false;
    }

    return true;
}

bool eicPresentationCreateEphemeralKeyPair(EicPresentation* ctx,
                                           uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE]) {
    if (!eicOpsCreateEcKey(ephemeralPrivateKey, ctx->ephemeralPublicKey)) {
        eicDebug("Error creating ephemeral key");
        return false;
    }
    return true;
}

bool eicPresentationCreateAuthChallenge(EicPresentation* ctx, uint64_t* authChallenge) {
    do {
        if (!eicOpsRandom((uint8_t*)authChallenge, sizeof(uint64_t))) {
            eicDebug("Failed generating random challenge");
            return false;
        }
    } while (*authChallenge != 0);
    return true;
}

// Setter for the top-level reader certificate
bool eicPresentationSetTopLevelReaderCert(EicPresentation* ctx, const uint8_t* certX509,
                                          size_t certX509Size) {
    eicDebug("TODO: not yet implemented");
    return true;
}

// Validates the next certificate in the reader certificate chain.
bool eicPresentationValidateNextReaderCert(EicPresentation* ctx, const uint8_t* certX509,
                                           size_t certX509Size) {
    eicDebug("TODO: not yet implemented");
    return true;
}

bool eicPresentationSetHardwareAuthToken(EicPresentation* ctx, uint64_t challenge,
                                         uint64_t secureUserId, uint64_t authenticatorId,
                                         int hardwareAuthenticatorType, uint64_t timeStamp,
                                         const uint8_t* mac, size_t macSize) {
    if (!eicOpsValidateHardwareAuthToken(challenge, secureUserId, authenticatorId,
                                         hardwareAuthenticatorType, timeStamp, mac, macSize)) {
        return false;
    }
    ctx->authTokenChallenge = challenge;
    ctx->authTokenSecureUserId = secureUserId;
    ctx->authTokenTimestamp = timeStamp;
    return true;
}

bool eicPresentationValidateAccessControlProfile(EicPresentation* ctx, int id,
                                                 const uint8_t* readerCertificate,
                                                 size_t readerCertificateSize,
                                                 bool userAuthenticationRequired, int timeoutMillis,
                                                 uint64_t secureUserId, const uint8_t mac[28],
                                                 bool* outAccessGranted) {
    *outAccessGranted = false;

    // TODO: implement reader authentication

    if (id < 0 || id >= 32) {
        eicDebug("id value of %d is out of allowed range [0, 32[", id);
        return false;
    }

    // Validate the MAC
    uint8_t cborBuffer[EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE];
    EicCbor cborBuilder;
    eicCborInit(&cborBuilder, cborBuffer, EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE);
    if (!eicCalcAccessControlCbor(&cborBuilder, id, readerCertificate, readerCertificateSize,
                                  userAuthenticationRequired, timeoutMillis, secureUserId)) {
        return false;
    }
    if (!eicOpsDecryptAes128Gcm(ctx->storageKey, mac, 28, cborBuilder.buffer, cborBuilder.size,
                                NULL)) {
        eicDebug("MAC for AccessControlProfile doesn't match");
        return false;
    }

    if (secureUserId != ctx->authTokenSecureUserId) {
        eicDebug("secureUserId in profile differs from userId in authToken");
        *outAccessGranted = false;
        return true;
    }

    if (userAuthenticationRequired) {
        if (timeoutMillis == 0) {
            if (ctx->authTokenChallenge == 0) {
                eicDebug("No challenge in authToken");
                *outAccessGranted = false;
                return true;
            }

            if (ctx->authTokenChallenge != ctx->authChallenge) {
                eicDebug("Challenge in authToken doesn't match the challenge we created");
                *outAccessGranted = false;
                return true;
            }
        } else {
            uint64_t now;
            if (!eicOpsGetTimestamp(&now)) {
                eicDebug("Error getting current time");
                return false;
            }
            if (ctx->authTokenTimestamp > now) {
                eicDebug("Timestamp in authToken is in the future");
                *outAccessGranted = false;
                return true;
            }
            if (now > ctx->authTokenTimestamp + timeoutMillis) {
                eicDebug("Deadline for authToken is in the past");
                *outAccessGranted = false;
                return true;
            }
        }
    }

    *outAccessGranted = true;
    ctx->accessControlProfileMask |= (1 << id);
    return true;
}

EicAccessCheckResult eicPresentationStartRetrieveEntryValue(EicPresentation* ctx,
                                                            const char* nameSpace, const char* name,
                                                            int32_t entrySize,
                                                            const int* accessControlProfileIds,
                                                            size_t numAccessControlProfileIds) {
    uint8_t additionalDataCbor[512];  // TODO: pick a size that's big enough
    size_t additionalDataCborSize;

    // We'll need to calc and store a digest of additionalData to check that it's the same
    // additionalData being passed in for every eicPresentationRetrieveEntryValue() call...
    if (!calcAdditionalData(accessControlProfileIds, numAccessControlProfileIds, nameSpace, name,
                            additionalDataCbor, sizeof additionalDataCbor, &additionalDataCborSize,
                            ctx->additionalDataSha256)) {
        return false;
    }

    // TODO: enforce access control

    return EIC_ACCESS_CHECK_RESULT_OK;
}

// Note: |content| must be big enough to hold |encryptedContentSize| - 28 bytes.
bool eicPresentationRetrieveEntryValue(EicPresentation* ctx, const uint8_t* encryptedContent,
                                       size_t encryptedContentSize, uint8_t* content,
                                       const char* nameSpace, const char* name,
                                       const int* accessControlProfileIds,
                                       size_t numAccessControlProfileIds) {
    uint8_t additionalDataCbor[512];  // TODO: pick a size that's big enough
    size_t additionalDataCborSize;

    uint8_t calculatedSha256[EIC_SHA256_DIGEST_SIZE];
    if (!calcAdditionalData(accessControlProfileIds, numAccessControlProfileIds, nameSpace, name,
                            additionalDataCbor, sizeof additionalDataCbor, &additionalDataCborSize,
                            calculatedSha256)) {
        return false;
    }
    if (eicMemCmp(calculatedSha256, ctx->additionalDataSha256, EIC_SHA256_DIGEST_SIZE) != 0) {
        eicDebug("SHA-256 mismatch of additionalData");
        return false;
    }

    if (!eicOpsDecryptAes128Gcm(ctx->storageKey, encryptedContent, encryptedContentSize,
                                additionalDataCbor, additionalDataCborSize, content)) {
        eicDebug("Error decrypting content");
        return false;
    }

    return true;
}
