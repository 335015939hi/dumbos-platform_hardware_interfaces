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

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.AeadOperationResult;
import android.hardware.security.see.hwcrypto.base_types.DmaAeadOperationResult;
import android.hardware.security.see.hwcrypto.base_types.DmaEmittingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.DmaOperationBuffers;
import android.hardware.security.see.hwcrypto.base_types.EmittingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.SymmetricOperationParameters;

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
    EmittingOperationResult begin(
            in HwCryptoKeyMaterial key, in SymmetricOperationParameters parameters);

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
    AeadOperationResult begin_aead(
            in HwCryptoKeyMaterial key, in SymmetricOperationParameters parameters);

    /*
     * begin_dma() - start a symmetric cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     * @dma_buffers:
     *      buffers to be used for the operation
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    DmaEmittingOperationResult begin_dma(in HwCryptoKeyMaterial key,
            in SymmetricOperationParameters parameters, in DmaOperationBuffers dma_buffers);

    /*
     * begin_dma_aead() - start an authenticated encryption with additional data
     *                                        cryptographic operation using DMA.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     * @dma_buffers:
     *      buffers to be used for the operation
     *
     * Return:
     *      Ok(IAeadOperation) on success, specific error code on error.
     */
    DmaAeadOperationResult begin_dma_aead(in HwCryptoKeyMaterial key,
            in SymmetricOperationParameters parameters, in DmaOperationBuffers dma_buffers);
}
