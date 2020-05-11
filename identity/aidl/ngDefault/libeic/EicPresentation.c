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

#include "EicPresentation.h"

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

// From "COSE Algorithms" registry
//
#define COSE_ALG_ECDSA_256 -7

bool eicPresentationValidateRequestMessage(EicPresentation* ctx, const uint8_t* sessionTranscript,
                                           size_t sessionTranscriptSize,
                                           const uint8_t* requestMessage, size_t requestMessageSize,
                                           int coseSignAlg,
                                           const uint8_t* readerSignatureOfToBeSigned,
                                           size_t readerSignatureOfToBeSignedSize) {
    if (ctx->readerPublicKeySize == 0) {
        eicDebug("No public key for reader");
        return false;
    }

    // TODO: validate SessionTranscript...

    // TODO: Support other algorithms than ECDSA-256 - ISO 18013-5 mentions
    // support for ES256, ES384, ES512, EdDSA...
    //
    // Right now we only support ECDSA with SHA-256 (e.g. ES256).
    //
    if (coseSignAlg != COSE_ALG_ECDSA_256) {
        eicDebug(
                "COSE Signature algorothm for reader signature is %d, "
                "only ECDSA with SHA-256 is supported right now",
                coseSignAlg);
        return false;
    }

    // What we're going to verify is the COSE ToBeSigned structure which
    // looks like the following:
    //
    //   Sig_structure = [
    //     context : "Signature" / "Signature1" / "CounterSignature",
    //     body_protected : empty_or_serialized_map,
    //     ? sign_protected : empty_or_serialized_map,
    //     external_aad : bstr,
    //     payload : bstr
    //    ]
    //
    // So we're going to build that CBOR...
    //
    EicCbor cbor;
    eicCborInit(&cbor, NULL, 0);
    eicCborAppendArray(&cbor, 4);
    eicCborAppendString(&cbor, "Signature1");

    // The COSE Encoded protected headers is just a single field with
    // COSE_LABEL_ALG (1) -> coseSignAlg (e.g. -7). For simplicitly we just
    // hard-code the CBOR encoding:
    static const uint8_t coseEncodedProtectedHeaders[] = {0xa1, 0x01, 0x26};
    eicCborAppendByteString(&cbor, coseEncodedProtectedHeaders,
                            sizeof(coseEncodedProtectedHeaders));

    // External_aad is the empty bstr
    static const uint8_t externalAad[0] = {};
    eicCborAppendByteString(&cbor, externalAad, sizeof(externalAad));

    // For the payload, the _encoded_ form follows here. We handle this by simply
    // opening a bstr, and then writing the CBOR. This requires us to know the
    // size of said bstr, ahead of time... the CBOR to be written is
    //
    //   ReaderAuthentication = [
    //      "ReaderAuthentication",
    //      SessionTranscript,
    //      ItemsRequestBytes
    //   ]
    //
    //   ItemsRequestBytes = #6.24(bstr .cbor ItemsRequest)
    //
    // which is easily calculated below
    //
    size_t calculatedSize = 0;
    calculatedSize += 1;  // Array of size 3
    calculatedSize += 1;  // "ReaderAuthentication" less than 24 bytes
    calculatedSize += sizeof("ReaderAuthentication") - 1;  // Don't include trailing NUL
    calculatedSize += sessionTranscriptSize;               // Already CBOR encoded
    calculatedSize += 2;  // Semantic tag EIC_CBOR_SEMANTIC_TAG_ENCODED_CBOR (24)
    calculatedSize += 1 + eicCborAdditionalLengthBytesFor(requestMessageSize);
    calculatedSize += requestMessageSize;
    eicCborBegin(&cbor, EIC_CBOR_MAJOR_TYPE_BYTE_STRING, calculatedSize);

    // And now that we know the size, let's fill it in...
    //
    size_t payloadOffset = cbor.size;
    eicCborBegin(&cbor, EIC_CBOR_MAJOR_TYPE_ARRAY, 3);
    eicCborAppendString(&cbor, "ReaderAuthentication");
    eicCborAppend(&cbor, sessionTranscript, sessionTranscriptSize);
    eicCborAppendSemantic(&cbor, EIC_CBOR_SEMANTIC_TAG_ENCODED_CBOR);
    eicCborBegin(&cbor, EIC_CBOR_MAJOR_TYPE_BYTE_STRING, requestMessageSize);
    eicCborAppend(&cbor, requestMessage, requestMessageSize);
    if (cbor.size != payloadOffset + calculatedSize) {
        eicDebug("CBOR size is %zd but we expected %zd", cbor.size, payloadOffset + calculatedSize);
        return false;
    }
    uint8_t toBeSignedDigest[EIC_SHA256_DIGEST_SIZE];
    eicCborFinal(&cbor, toBeSignedDigest);

    if (!eicOpsEcDsaVerifyWithPublicKey(
                toBeSignedDigest, EIC_SHA256_DIGEST_SIZE, readerSignatureOfToBeSigned,
                readerSignatureOfToBeSignedSize, ctx->readerPublicKey, ctx->readerPublicKeySize)) {
        eicDebug("Request message is not signed by public key");
        return false;
    }
    ctx->requestMessageValidated = true;
    return true;
}

// Validates the next certificate in the reader certificate chain.
bool eicPresentationPushReaderCert(EicPresentation* ctx, const uint8_t* certX509,
                                   size_t certX509Size) {
    // If we had a previous certificate, use its public key to validate this certificate.
    if (ctx->readerPublicKeySize > 0) {
        if (!eicOpsX509CertSignedByPublicKey(certX509, certX509Size, ctx->readerPublicKey,
                                             ctx->readerPublicKeySize)) {
            eicDebug("Certificate is not signed by public key in the previous certificate");
            return false;
        }
    }

    // Store the key of this certificate, this is used to validate the next certificate
    // and also ACPs with certificates that use the same public key...
    ctx->readerPublicKeySize = EIC_PRESENTATION_MAX_READER_PUBLIC_KEY_SIZE;
    if (!eicOpsX509GetPublicKey(certX509, certX509Size, ctx->readerPublicKey,
                                &ctx->readerPublicKeySize)) {
        eicDebug("Error extracting public key from certificate");
        return false;
    }
    if (ctx->readerPublicKeySize == 0) {
        eicDebug("Zero-length public key in certificate");
        return false;
    }

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

static bool checkUserAuth(EicPresentation* ctx, bool userAuthenticationRequired, int timeoutMillis,
                          uint64_t secureUserId) {
    if (!userAuthenticationRequired) {
        return true;
    }

    if (secureUserId != ctx->authTokenSecureUserId) {
        eicDebug("secureUserId in profile differs from userId in authToken");
        return false;
    }

    if (timeoutMillis == 0) {
        if (ctx->authTokenChallenge == 0) {
            eicDebug("No challenge in authToken");
            return false;
        }

        if (ctx->authTokenChallenge != ctx->authChallenge) {
            eicDebug("Challenge in authToken doesn't match the challenge we created");
            return false;
        }
    }

    uint64_t now;
    if (!eicOpsGetTimestamp(&now)) {
        eicDebug("Error getting current time");
        return false;
    }
    if (ctx->authTokenTimestamp > now) {
        eicDebug("Timestamp in authToken is in the future");
        return false;
    }
    if (now > ctx->authTokenTimestamp + timeoutMillis) {
        eicDebug("Deadline for authToken is in the past");
        return false;
    }

    return true;
}

static bool checkReaderAuth(EicPresentation* ctx, const uint8_t* readerCertificate,
                            size_t readerCertificateSize) {
    uint8_t publicKey[EIC_PRESENTATION_MAX_READER_PUBLIC_KEY_SIZE];
    size_t publicKeySize;

    if (readerCertificateSize == 0) {
        return true;
    }

    // Remember in this case certificate equality is done by comparing public
    // keys, not bitwise comparison of the certificates.
    //
    publicKeySize = EIC_PRESENTATION_MAX_READER_PUBLIC_KEY_SIZE;
    if (!eicOpsX509GetPublicKey(readerCertificate, readerCertificateSize, publicKey,
                                &publicKeySize)) {
        eicDebug("Error extracting public key from certificate");
        return false;
    }
    if (publicKeySize == 0) {
        eicDebug("Zero-length public key in certificate");
        return false;
    }

    if ((ctx->readerPublicKeySize != publicKeySize) ||
        (eicMemCmp(ctx->readerPublicKey, publicKey, ctx->readerPublicKeySize) != 0)) {
        return false;
    }
    return true;
}

// Note: This function returns false _only_ if an error occurred check for access, _not_
// whether access is granted. Whether access is granted is returned in |accessGranted|.
//
bool eicPresentationValidateAccessControlProfile(EicPresentation* ctx, int id,
                                                 const uint8_t* readerCertificate,
                                                 size_t readerCertificateSize,
                                                 bool userAuthenticationRequired, int timeoutMillis,
                                                 uint64_t secureUserId, const uint8_t mac[28],
                                                 bool* accessGranted) {
    *accessGranted = false;

    if (id < 0 || id >= 32) {
        eicDebug("id value of %d is out of allowed range [0, 32[", id);
        return false;
    }

    // Validate the MAC
    uint8_t cborBuffer[EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE];
    EicCbor cborBuilder;
    eicCborInit(&cborBuilder, cborBuffer, EIC_MAX_CBOR_SIZE_FOR_ACCESS_CONTROL_PROFILE);
    if (!eicCborCalcAccessControl(&cborBuilder, id, readerCertificate, readerCertificateSize,
                                  userAuthenticationRequired, timeoutMillis, secureUserId)) {
        return false;
    }
    if (!eicOpsDecryptAes128Gcm(ctx->storageKey, mac, 28, cborBuilder.buffer, cborBuilder.size,
                                NULL)) {
        eicDebug("MAC for AccessControlProfile doesn't match");
        return false;
    }

    bool passedUserAuth =
            checkUserAuth(ctx, userAuthenticationRequired, timeoutMillis, secureUserId);
    bool passedReaderAuth = checkReaderAuth(ctx, readerCertificate, readerCertificateSize);

    ctx->accessControlProfileMaskValidated |= (1 << id);
    if (readerCertificateSize > 0) {
        ctx->accessControlProfileMaskUsesReaderAuth |= (1 << id);
    }
    if (!passedReaderAuth) {
        ctx->accessControlProfileMaskFailedReaderAuth |= (1 << id);
    }
    if (!passedUserAuth) {
        ctx->accessControlProfileMaskFailedUserAuth |= (1 << id);
    }

    if (passedUserAuth && passedReaderAuth) {
        *accessGranted = true;
        eicDebug("Access granted for id %d", id);
    }
    return true;
}

bool eicPresentationStartRetrieveEntries(EicPresentation* ctx) {
    // HAL may use this object multiple times to retrieve data so need to reset various
    // state objects here.
    ctx->requestMessageValidated = false;
    ctx->accessControlProfileMaskValidated = 0;
    ctx->accessControlProfileMaskUsesReaderAuth = 0;
    ctx->accessControlProfileMaskFailedReaderAuth = 0;
    ctx->accessControlProfileMaskFailedUserAuth = 0;
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
    if (!eicCborCalcEntryAdditionalData(accessControlProfileIds, numAccessControlProfileIds,
                                        nameSpace, name, additionalDataCbor,
                                        sizeof additionalDataCbor, &additionalDataCborSize,
                                        ctx->additionalDataSha256)) {
        return EIC_ACCESS_CHECK_RESULT_FAILED;
    }

    if (numAccessControlProfileIds == 0) {
        return EIC_ACCESS_CHECK_RESULT_NO_ACCESS_CONTROL_PROFILES;
    }

    // Access is granted if at least one of the profiles grants access.
    //
    // If an item is configured without any profiles, access is denied.
    //
    EicAccessCheckResult result = EIC_ACCESS_CHECK_RESULT_FAILED;
    for (size_t n = 0; n < numAccessControlProfileIds; n++) {
        int id = accessControlProfileIds[n];
        uint32_t idBitMask = (1 << id);

        // If the access control profile wasn't validated, this is an error and we
        // fail immediately.
        bool validated = ((ctx->accessControlProfileMaskValidated & idBitMask) != 0);
        if (!validated) {
            eicDebug("No ACP for profile id %d", id);
            return EIC_ACCESS_CHECK_RESULT_FAILED;
        }

        // If the HAL continues without eicPresentationValidateRequestMessage()
        // having signaled success and tries to retrieve data items requiring
        // reader auth, fail hard.
        //
        // Only a buggy or compromised HAL would do this. However failure to
        // protect against this would leave us gullible to release data where
        // the HAL uses a certificate chain stolen from an authorized reader.
        //
        bool usesReaderAuth = ((ctx->accessControlProfileMaskUsesReaderAuth & idBitMask) != 0);
        if (usesReaderAuth && !ctx->requestMessageValidated) {
            eicDebug("Trying to retrieve reader authentication without validated requestMessage");
            return EIC_ACCESS_CHECK_RESULT_FAILED;
        }

        // Otherwise, we _did_ validate the profile. If none of the checks
        // failed, we're done
        bool failedUserAuth = ((ctx->accessControlProfileMaskFailedUserAuth & idBitMask) != 0);
        bool failedReaderAuth = ((ctx->accessControlProfileMaskFailedReaderAuth & idBitMask) != 0);
        if (!failedUserAuth && !failedReaderAuth) {
            result = EIC_ACCESS_CHECK_RESULT_OK;
            break;
        }
        // One of the checks failed, convey which one
        if (failedUserAuth) {
            result = EIC_ACCESS_CHECK_RESULT_USER_AUTHENTICATION_FAILED;
        } else {
            result = EIC_ACCESS_CHECK_RESULT_READER_AUTHENTICATION_FAILED;
        }
    }
    eicDebug("Result %d for name %s", result, name);
    return result;
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
    if (!eicCborCalcEntryAdditionalData(accessControlProfileIds, numAccessControlProfileIds,
                                        nameSpace, name, additionalDataCbor,
                                        sizeof additionalDataCbor, &additionalDataCborSize,
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
