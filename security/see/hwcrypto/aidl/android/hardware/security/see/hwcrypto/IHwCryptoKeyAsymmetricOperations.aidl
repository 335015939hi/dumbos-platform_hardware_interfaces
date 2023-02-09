package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.AccumulatingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.AsymmetricSharedSecretParameters;
import android.hardware.security.see.hwcrypto.base_types.AsymmetricSignParameters;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

interface IHwCryptoKeyAsymmetricOperations {
    /*
     * XXXXX() - start a symmetric cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IEmittingOperation) on success, specific error code on error.
     */
    AccumulatingOperationResult begin_sign(
            in HwCryptoKeyMaterial key, in AsymmetricSignParameters parameters);

    /*
     * XXXXX() - start an authenticated encryption with additional data
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
    AccumulatingOperationResult begin_shared_secret(
            in HwCryptoKeyMaterial key, in AsymmetricSharedSecretParameters parameters);

    /*
     * XXXXX() - start an authenticated encryption with additional data
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
    VectorResult get_public_key(in HwCryptoKeyMaterial key);
}
