package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.AeadOperationResult;
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
    EmittingOperationResult begin_dma(in HwCryptoKeyMaterial key,
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
    AeadOperationResult begin_dma_aead(in HwCryptoKeyMaterial key,
            in SymmetricOperationParameters parameters, in DmaOperationBuffers dma_buffers);
}
