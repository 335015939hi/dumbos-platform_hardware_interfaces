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

import android.hardware.security.see.hwcrypto.types.CipherModeParameters;
import android.hardware.security.see.hwcrypto.types.Void;

/*
 * Type used for the parameters needed to run a triple DES operation. Its use is not recommended
 * other than for compatibility purposes with existing code.
 */
union TDesCipherMode {
    /*
     * Electronic Code Block mode. No padding will be added so input size needs to be a multiple of
     * a triple DES block size (8 bytes).
     */
    @nullable Void ecbNoPadding;

    /*
     * Electronic Code Block mode. It will be padded using PKCS#7.
     */
    CipherModeParameters ecbPkcs7Padding;

    /*
     * Cipher Block Chaining mode. No padding will be added so input size needs to be a multiple of
     * a triple DES block size (8 bytes) and it contains the nonce for the operation.
     */
    @nullable Void cbcNoPadding;

    /*
     * Cipher Block Chaining mode. It will be padded using PKCS#7 and it contains the nonce for the
     * operation.
     */
    CipherModeParameters cbcPkcs7Padding;
}
