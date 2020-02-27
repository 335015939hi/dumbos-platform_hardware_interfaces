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

void eicCborInit(EicCbor* cbor, size_t bufferSize) {
    cbor->size = 0;
    cbor->bufferSize = bufferSize;
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

void eicProvisionInit(EicProvision* ctx, const char* docType) {
    eicCborInit(&ctx->cbor, 0);

    eicCborAppendArray(&ctx->cbor, 5);
    eicCborAppendString(&ctx->cbor, "ProofOfProvisioning");
    eicCborAppendString(&ctx->cbor, docType);
}

bool eicStartPersonalization(EicProvision* ctx, int accessControlProfileCount,
                             const int* entryCounts, size_t numEntryCounts) {
    ctx->accessControlProfileCount = accessControlProfileCount;
    ctx->numEntryCounts = numEntryCounts;
    if (numEntryCounts > EIC_MAX_NUM_NAMESPACES) {
        return false;
    }
    for (size_t n = 0; n < numEntryCounts; n++) {
        ctx->entryCounts[n] = entryCounts[n];
    }
    ctx->curNamespace = -1;
    ctx->curNamespaceNumProcessed = 0;

    eicCborAppendArray(&ctx->cbor, accessControlProfileCount);

    return true;
}

bool eicAddAccessControlProfile(EicProvision* ctx, int id, const uint8_t* readerCertificate,
                                size_t readerCertificateSize, bool userAuthenticationRequired,
                                uint64_t timeoutMillis, uint64_t secureUserId) {
    if (userAuthenticationRequired) {
        if (readerCertificateSize > 0) {
            eicCborAppendMap(&ctx->cbor, 4);
            eicCborAppendString(&ctx->cbor, "id");
            eicCborAppendUnsigned(&ctx->cbor, id);
            eicCborAppendString(&ctx->cbor, "readerCertificate");
            eicCborAppendByteString(&ctx->cbor, readerCertificate, readerCertificateSize);
            eicCborAppendString(&ctx->cbor, "userAuthenticationRequired");
            eicCborAppendUnsigned(&ctx->cbor, userAuthenticationRequired);
            eicCborAppendString(&ctx->cbor, "timeoutMillis");
            eicCborAppendUnsigned(&ctx->cbor, timeoutMillis);
        } else {
            eicCborAppendMap(&ctx->cbor, 3);
            eicCborAppendString(&ctx->cbor, "id");
            eicCborAppendUnsigned(&ctx->cbor, id);
            eicCborAppendString(&ctx->cbor, "userAuthenticationRequired");
            eicCborAppendUnsigned(&ctx->cbor, userAuthenticationRequired);
            eicCborAppendString(&ctx->cbor, "timeoutMillis");
            eicCborAppendUnsigned(&ctx->cbor, timeoutMillis);
        }
    } else {
        if (readerCertificateSize > 0) {
            eicCborAppendMap(&ctx->cbor, 2);
            eicCborAppendString(&ctx->cbor, "id");
            eicCborAppendUnsigned(&ctx->cbor, id);
            eicCborAppendString(&ctx->cbor, "readerCertificate");
            eicCborAppendByteString(&ctx->cbor, readerCertificate, readerCertificateSize);
        } else {
            eicCborAppendMap(&ctx->cbor, 1);
            eicCborAppendString(&ctx->cbor, "id");
            eicCborAppendUnsigned(&ctx->cbor, id);
        }
    }
    // TODO: return MAC
    return true;
}

bool eicBeginAddEntry(EicProvision* ctx, const int* accessControlProfileIds,
                      size_t numAccessControlProfileIds, const char* nameSpace, const char* name,
                      uint64_t entrySize) {
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

    // Unfortunately we need to make a copy of access control profiles so they can
    // be set after "value" map entry
    if (numAccessControlProfileIds > EIC_MAX_NUM_ACCESS_CONTROL_PROFILE_IDS) {
        return false;
    }
    ctx->curNumAccessControlProfileIds = numAccessControlProfileIds;
    for (size_t n = 0; n < numAccessControlProfileIds; n++) {
        ctx->curAccessControlProfileIds[n] = accessControlProfileIds[n];
    }
    ctx->curEntrySize = entrySize;
    ctx->curEntryNumBytesReceived = 0;

    eicCborAppendString(&ctx->cbor, "value");

    ctx->curNamespaceNumProcessed += 1;
    return true;
}

bool eicAddEntryValue(EicProvision* ctx, const uint8_t* content, size_t contentSize) {
    eicCborAppend(&ctx->cbor, content, contentSize);

    // If done with this entry, close the map
    ctx->curEntryNumBytesReceived += contentSize;
    if (ctx->curEntryNumBytesReceived == ctx->curEntrySize) {
        eicCborAppendString(&ctx->cbor, "accessControlProfiles");
        eicCborAppendArray(&ctx->cbor, ctx->curNumAccessControlProfileIds);
        for (size_t n = 0; n < ctx->curNumAccessControlProfileIds; n++) {
            eicCborAppendNumber(&ctx->cbor, ctx->curAccessControlProfileIds[n]);
        }
    }
    return true;
}

bool eicFinishAddingEntries(EicProvision* ctx, bool testCredential,
                            uint8_t cborSha256[EIC_SHA256_DIGEST_SIZE]) {
    eicCborAppendBool(&ctx->cbor, testCredential);
    eicCborFinal(&ctx->cbor, cborSha256);
    return true;
}
