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

import android.hardware.security.see.hwcrypto.types.CipherModeParameters;
import android.hardware.security.see.hwcrypto.types.Void;

/*
 * Type used for the parameters needed to run a non-authenticated AES operation.
 */
union AesCipherMode {
    /*
     * Electronic Code Block mode, its use is not recommended other than for compatibility purposes
     * with existing code. No padding will be added so input size needs to be a multiple of an AES
     * block size (16 bytes).
     */
    @nullable Void ecbNoPadding;

    /*
     * Electronic Code Block mode, its use is not recommended other than for compatibility purposes
     * with existing code. It will be padded using PKCS#7.
     */
    @nullable Void ecbPkcs7Padding;

    /*
     * Cipher Block Chaining mode. No padding will be added so input size needs to be a multiple of
     * an AESblock size (16 bytes) and it contains the nonce for the operation.
     */
    CipherModeParameters cbcNoPadding;

    /*
     * Cipher Block Chaining mode. It will be padded using PKCS#7 and it contains the nonce for the
     * operation.
     */
    CipherModeParameters cbcPkcs7Padding;

    /*
     * Counter mode. Type contains the nonce for the operation.
     */
    CipherModeParameters ctr;
}
