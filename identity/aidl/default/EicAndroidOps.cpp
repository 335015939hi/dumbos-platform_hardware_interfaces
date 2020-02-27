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

#include <openssl/sha.h>

#include "EicAndroidOps.h"

#define LOG_TAG "EicAndroidOps"

static void* allocMemory(size_t numBytes) {
    uint8_t* ptr = new uint8_t[numBytes];
    for (size_t n = 0; n < numBytes; n++) {
        ptr[n] = '\0';
    }
    return ptr;
}

static void freeMemory(void* ptr) {
    delete[]((uint8_t*)ptr);
}

struct EicSha256Ctx {
    SHA256_CTX ctx;
};

static EicSha256Ctx* sha256New(void) {
    EicSha256Ctx* ctx = new EicSha256Ctx;
    SHA256_Init(&ctx->ctx);
    return ctx;
}

static void sha256Update(EicSha256Ctx* ctx, const uint8_t* data, size_t len) {
    SHA256_Update(&ctx->ctx, data, len);
}

static void sha256Final(EicSha256Ctx* ctx, uint8_t digest[EIC_SHA256_DIGEST_SIZE]) {
    SHA256_Final(digest, &ctx->ctx);
    delete ctx;
}

namespace aidl::android::hardware::identity {

EicOps* eicAndroidOpsNew() {
    EicOps* ops = new EicOps;
    ops->allocMemory = allocMemory;
    ops->freeMemory = freeMemory;
    ops->sha256New = sha256New;
    ops->sha256Update = sha256Update;
    ops->sha256Final = sha256Final;
    return ops;
}

void eicAndroidOpsFree(EicOps* ops) {
    delete ops;
}

}  // namespace aidl::android::hardware::identity
