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

#include "EicCbor.h"

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

size_t eicCborAdditionalLengthBytesFor(size_t size) {
    if (size < 24) {
        return 0;
    } else if (size <= 0xff) {
        return 1;
    } else if (size <= 0xffff) {
        return 2;
    } else if (size <= 0xffffffff) {
        return 4;
    }
    return 8;
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

void eicCborAppendSemantic(EicCbor* cbor, uint64_t value) {
    size_t encoded = value;
    eicCborBegin(cbor, EIC_CBOR_MAJOR_TYPE_SEMANTIC, encoded);
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

bool eicCborCalcAccessControl(EicCbor* cborBuilder, int id, const uint8_t* readerCertificate,
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

bool eicCborCalcEntryAdditionalData(const int* accessControlProfileIds,
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
