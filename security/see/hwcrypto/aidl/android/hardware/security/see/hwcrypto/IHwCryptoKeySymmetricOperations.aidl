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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.IAeadOperation;
import android.hardware.security.see.hwcrypto.IDmaAeadOperation;
import android.hardware.security.see.hwcrypto.IDmaEmittingOperation;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.types.SymmetricOperationParameters;

// TODO: @nullable cannot be used on enums, so using wrappers for them for now

interface IHwCryptoKeySymmetricOperations {
    /*
     * begin() - start a symmetric cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    IEmittingOperation begin(in OpaqueKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_aead() - start an authenticated encryption with additional data
     *                                    cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IAeadOperation) on success, specific error code on error.
     */
    IAeadOperation begin_aead(in OpaqueKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_dma() - start a symmetric cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    IDmaEmittingOperation begin_dma(
            in OpaqueKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_dma_aead() - start an authenticated encryption with additional data
     *                                        cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IAeadOperation) on success, specific error code on error.
     */
    IDmaAeadOperation begin_dma_aead(
            in OpaqueKeyMaterial key, in SymmetricOperationParameters parameters);
}
