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

#define EIC_SHA256_DIGEST_SIZE 32

struct EicSha256Ctx;
typedef struct EicSha256Ctx EicSha256Ctx;

typedef struct EicOps {
    // Memory allocation.
    void* (*allocMemory)(size_t numBytes);
    void (*freeMemory)(void* ptr);

    // SHA-256 functions.
    EicSha256Ctx* (*sha256New)(void);
    void (*sha256Update)(EicSha256Ctx* ctx, const uint8_t* data, size_t len);
    void (*sha256Final)(EicSha256Ctx* ctx, uint8_t digest[EIC_SHA256_DIGEST_SIZE]);
} EicOps;

struct EicProvision;
typedef struct EicProvision EicProvision;

EicProvision* eicProvisionNew(EicOps* ops, const char* docType, bool testCredential);

bool eicStartPersonalization(EicProvision* ctx, int accessControlProfileCount,
                             const int* entryCounts, size_t numEntryCounts);

// TODO: return MAC
bool eicAddAccessControlProfile(EicProvision* ctx, int id, const uint8_t* readerCertificate,
                                size_t readerCertificateSize, bool userAuthenticationRequired,
                                uint64_t timeoutMillis, uint64_t secureUserId);

bool eicBeginAddEntry(EicProvision* ctx, const int* accessControlProfileIds,
                      size_t numAccessControlProfileIds, const char* nameSpace, const char* name,
                      uint64_t entrySize);

// TODO: return encrypted content
bool eicAddEntryValue(EicProvision* ctx, const uint8_t* content, size_t contentSize);

bool eicFinishAddingEntries(EicProvision* ctx, uint8_t cborSha256[EIC_SHA256_DIGEST_SIZE]);

void eicProvisionFree(EicProvision* ctx);

#ifdef __cplusplus
}
#endif

#endif  // ANDROID_HARDWARE_IDENTITY_EMBEDDED_IC_H
