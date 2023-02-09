/*
 * Copyright 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package android.hardware.security.see.hwcrypto.types;

enum KeyType {
    AES_128_ECB,
    AES_128_CBC,
    AES_128_CTR,
    AES_128_GCM,
    AES_128_XTS,
    AES_128_CMAC,
    AES_128_KEY_WRAP,
    AES_192_ECB,
    AES_192_CBC,
    AES_192_CTR,
    AES_192_GCM,
    AES_192_XTS,
    AES_192_CMAC,
    AES_192_KEY_WRAP,
    AES_256_ECB,
    AES_256_CBC,
    AES_256_CTR,
    AES_256_GCM,
    AES_256_XTS,
    AES_256_CMAC,
    AES_256_KEY_WRAP,
    TDES_ECB,
    TDES_CBC,
    HMAC_SHA224,
    HMAC_SHA256,
    HMAC_SHA384,
    HMAC_SHA512,
    RSA2048,
    RSA3072,
    RSA4096,
    ECC_NIST_P224,
    ECC_NIST_P256,
    ECC_NIST_P384,
    ECC_NIST_P521,
    ECC_ED25519,
    ECC_X25519,
}
