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

#include <inttypes.h>

#include "EicSession.h"

static uint64_t gSessionCount = 1;

static EicSession* gCurSession;

EicSession* eicSessionGetForId(uint64_t sessionId) {
    if (gCurSession != NULL && gCurSession->id == sessionId) {
        return gCurSession;
    }
    return NULL;
}

bool eicSessionInit(EicSession* ctx) {
    gCurSession = ctx;

    eicMemSet(ctx, '\0', sizeof(EicSession));

    ctx->id = gSessionCount++;  // TODO: abstract in eicOps

    do {
        if (!eicOpsRandom((uint8_t*)&(ctx->authChallenge), sizeof(uint64_t))) {
            eicDebug("Failed generating random challenge");
            return false;
        }
    } while (ctx->authChallenge == 0);

    if (!eicOpsCreateEcKey(ctx->ephemeralPrivateKey, ctx->ephemeralPublicKey)) {
        eicDebug("Error creating ephemeral key-pair");
        return false;
    }

    return true;
}

bool eicSessionGetId(EicSession* ctx, uint64_t* outId) {
    *outId = ctx->id;
    return true;
}

bool eicSessionGetAuthChallenge(EicSession* ctx, uint64_t* outAuthChallenge) {
    *outAuthChallenge = ctx->authChallenge;
    return true;
}

bool eicSessionGetEphemeralKeyPair(EicSession* ctx,
                                   uint8_t ephemeralPrivateKey[EIC_P256_PRIV_KEY_SIZE]) {
    eicMemCpy(ephemeralPrivateKey, ctx->ephemeralPrivateKey, EIC_P256_PRIV_KEY_SIZE);
    return true;
}

bool eicSessionSetReaderEphemeralPublicKey(
        EicSession* ctx, const uint8_t readerEphemeralPublicKey[EIC_P256_PUB_KEY_SIZE]) {
    eicMemCpy(ctx->readerEphemeralPublicKey, readerEphemeralPublicKey, EIC_P256_PUB_KEY_SIZE);
    return true;
}

bool eicSessionSetSessionTranscript(EicSession* ctx, const uint8_t* sessionTranscript,
                                    size_t sessionTranscriptSize) {
    // Only accept the SessionTranscript if X and Y from the ephemeral key
    // we created is somewhere in SessionTranscript...
    //
    if (eicMemMem(sessionTranscript, sessionTranscriptSize, ctx->ephemeralPublicKey,
                  EIC_P256_PUB_KEY_SIZE / 2) == NULL) {
        eicDebug("Error finding X from ephemeralPublicKey in sessionTranscript");
        return false;
    }
    if (eicMemMem(sessionTranscript, sessionTranscriptSize,
                  ctx->ephemeralPublicKey + EIC_P256_PUB_KEY_SIZE / 2,
                  EIC_P256_PUB_KEY_SIZE / 2) == NULL) {
        eicDebug("Error finding Y from ephemeralPublicKey in sessionTranscript");
        return false;
    }

    // To save space we only store the SHA-256 of SessionTranscript
    //
    EicSha256Ctx shaCtx;
    eicOpsSha256Init(&shaCtx);
    eicOpsSha256Update(&shaCtx, sessionTranscript, sessionTranscriptSize);
    eicOpsSha256Final(&shaCtx, ctx->sessionTranscriptSha256);
    return true;
}
