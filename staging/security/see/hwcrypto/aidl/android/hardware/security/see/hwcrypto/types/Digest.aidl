/*
 * Copyright 2024 The Android Open Source Project
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

/*
 * Enum that describes the type of cryptographic digest to be used by an operation like signing.
 */
enum Digest {
    /*
     * No Digest to be used.
     */
    NONE,

    /*
     * SHA2 message digest with length of 256 bits.
     */
    SHA256,

    /*
     * SHA2 message digest with length of 512 bits.
     */
    SHA512,
}
