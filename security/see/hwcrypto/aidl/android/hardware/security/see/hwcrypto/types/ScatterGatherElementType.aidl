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

/*
 * enum ScatterGatherElementType - Enum used to describe the content of the ScatterGatherElement. It
 *                                 it only has meaning if used for inputs to encrypt operations.
 *
 * @ENCRYPTED_BUFFER:
 *      Buffer is encrypted and should be decrypted.
 * @CLEAR_BUFFER:
 *      Buffer is not encrypted and should be just copied.
 * @AAD:
 *      Additional Authenticated Data for AEAD. Notice that all AAD data needs to be provided before
 *      providing any encrypted data. Once encrypted data has started to be processed, no additional
 *      AAD can be provided.
 */
enum ScatterGatherElementType {
    ENCRYPTED_BUFFER,
    CLEAR_BUFFER,
    AAD,
}
