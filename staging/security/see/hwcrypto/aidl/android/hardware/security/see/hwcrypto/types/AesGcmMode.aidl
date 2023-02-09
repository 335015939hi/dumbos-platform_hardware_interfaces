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

import android.hardware.security.see.hwcrypto.types.AesGcmModeParameters;

/*
 * Type used for the parameters needed to run an authenticated AES operation (GCM).
 */
union AesGcmMode {
    /*
     * Galois Counter Mode with an authentication Tag that has a length of 12 bytes. Internal type
     * contains the operation Nonce.
     */
    AesGcmModeParameters gcmTag12;

    /*
     * Galois Counter Mode with an authentication Tag that has a length of 13 bytes. Internal type
     * contains the operation Nonce.
     */
    AesGcmModeParameters gcmTag13;

    /*
     * Galois Counter Mode with an authentication Tag that has a length of 14 bytes. Internal type
     * contains the operation Nonce.
     */
    AesGcmModeParameters gcmTag14;

    /*
     * Galois Counter Mode with an authentication Tag that has a length of 15 bytes. Internal type
     * contains the operation Nonce.
     */
    AesGcmModeParameters gcmTag15;

    /*
     * Galois Counter Mode with an authentication Tag that has a length of 16 bytes. Internal type
     * contains the operation Nonce.
     */
    AesGcmModeParameters gcmTag16;
}
