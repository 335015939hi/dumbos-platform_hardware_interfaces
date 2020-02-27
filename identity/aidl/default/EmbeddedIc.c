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

#define EIC_DOCTYPE_MAX_SIZE 256

#define EIC_MAX_NUM_NAMESPACES 32

#define EIC_MAX_CBOR_SIZE 4096

#define EIC_MAX_NUM_ACCESS_CONTROL_PROFILE_IDS 32

static size_t eic_strlen(const char* str) {
    size_t n = 0;
    for (n = 0; str[n] != '\0'; n++) {
        /* do nothing */
    }
    return n;
}

typedef struct {
    uint8_t data[EIC_MAX_CBOR_SIZE];
    size_t size;
    EicOps* ops;
    EicSha256Ctx* digester;
} EicCbor;

void eicCborInit(EicCbor* cbor, EicOps* ops) {
    cbor->size = 0;
    cbor->ops = ops;
    cbor->digester = ops->sha256New();
}

void eicCborFinal(EicCbor* cbor, uint8_t digest[EIC_SHA256_DIGEST_SIZE]) {
    if (cbor->size > 0) {
        cbor->ops->sha256Update(cbor->digester, cbor->data, cbor->size);
        cbor->size = 0;
    }
    cbor->ops->sha256Final(cbor->digester, digest);
}

void eicCborAppend(EicCbor* cbor, const uint8_t* data, size_t size) {
    for (size_t n = 0; n < size; n++) {
        cbor->data[cbor->size] = data[n];
        cbor->size += 1;
        if (cbor->size >= EIC_MAX_CBOR_SIZE) {
            cbor->ops->sha256Update(cbor->digester, cbor->data, cbor->size);
            cbor->size = 0;
        }
    }
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

#define EIC_CBOR_MAJOR_TYPE_UNSIGNED 0
#define EIC_CBOR_MAJOR_TYPE_NEGATIVE 1
#define EIC_CBOR_MAJOR_TYPE_BYTE_STRING 2
#define EIC_CBOR_MAJOR_TYPE_STRING 3
#define EIC_CBOR_MAJOR_TYPE_ARRAY 4
#define EIC_CBOR_MAJOR_TYPE_MAP 5
#define EIC_CBOR_MAJOR_TYPE_SIMPLE 7

#define EIC_CBOR_SIMPLE_VALUE_FALSE 20
#define EIC_CBOR_SIMPLE_VALUE_TRUE 21

void eicCborAppendByteString(EicCbor* cbor, const uint8_t* data, size_t dataSize) {
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_BYTE_STRING, dataSize);
    eicCborAppend(cbor, data, dataSize);
}

void eicCborAppendString(EicCbor* cbor, const char* str) {
    size_t length = eic_strlen(str);
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

struct EicProvision {
    EicOps* ops;
    char docType[EIC_DOCTYPE_MAX_SIZE];
    bool testCredential;
    int numEntryCounts;
    int entryCounts[EIC_MAX_NUM_NAMESPACES];
    int accessControlProfileCount;

    size_t curNumAccessControlProfileIds;
    int curAccessControlProfileIds[EIC_MAX_NUM_ACCESS_CONTROL_PROFILE_IDS];

    int curNamespace;
    int curNamespaceNumProcessed;
    size_t curEntrySize;
    size_t curEntryNumBytesReceived;

    EicCbor cbor;
};

static bool eicCopyStr(char* dest, size_t destSize, const char* src) {
    size_t n = 0;
    while (n < destSize && src[n] != '\0') {
        dest[n] = src[n];
        n++;
    }
    if (n == destSize) {
        return false;
    }
    dest[n] = '\0';
    return true;
}

EicProvision* eicProvisionNew(EicOps* ops, const char* docType, bool testCredential) {
    EicProvision* ctx = ops->allocMemory(sizeof(EicProvision));
    if (ctx == NULL) {
        goto out;
    }
    ctx->ops = ops;
    if (!eicCopyStr(ctx->docType, EIC_DOCTYPE_MAX_SIZE, docType)) {
        goto out;
    }
    ctx->testCredential = testCredential;

    eicCborInit(&ctx->cbor, ops);

    eicCborAppendArray(&ctx->cbor, 5);
    eicCborAppendString(&ctx->cbor, "ProofOfProvisioning");
    eicCborAppendString(&ctx->cbor, docType);

    return ctx;

out:
    if (ctx != NULL) {
        eicProvisionFree(ctx);
    }
    return NULL;
}

void eicProvisionFree(EicProvision* ctx) {
    ctx->ops->freeMemory(ctx);
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

bool eicFinishAddingEntries(EicProvision* ctx, uint8_t cborSha256[EIC_SHA256_DIGEST_SIZE]) {
    eicCborAppendBool(&ctx->cbor, ctx->testCredential);
    eicCborFinal(&ctx->cbor, cborSha256);
    return true;
}
