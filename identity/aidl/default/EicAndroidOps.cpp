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

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <android-base/logging.h>
#include <string.h>

#include <openssl/sha.h>

#include "EmbeddedIc.h"

#define LOG_TAG "EicAndroidOps"

using ::std::optional;
using ::std::string;
using ::std::tuple;
using ::std::vector;

void* eicMemCpy(void* dest, const void* src, size_t n) {
    return memcpy(dest, src, n);
}

size_t eicStrLen(const char* s) {
    return strlen(s);
}

void eicOpsSha256Init(EicSha256Ctx* ctx) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Init(realCtx);
}

void eicOpsSha256Update(EicSha256Ctx* ctx, const uint8_t* data, size_t len) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Update(realCtx, data, len);
}

void eicOpsSha256Final(EicSha256Ctx* ctx, uint8_t digest[EIC_SHA256_DIGEST_SIZE]) {
    SHA256_CTX* realCtx = (SHA256_CTX*)ctx;
    SHA256_Final(digest, realCtx);
}

bool eicOpsRandom(uint8_t* buf, size_t numBytes) {
    optional<vector<uint8_t>> bytes = ::android::hardware::identity::support::getRandom(numBytes);
    if (!bytes.has_value()) {
        return false;
    }
    memcpy(buf, bytes.value().data(), numBytes);
    return true;
}
