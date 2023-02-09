package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.AccumulatingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.RsaSharedSecretParameters;
import android.hardware.security.see.hwcrypto.base_types.RsaSignParameters;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

interface IHwCryptoKeyRsaOperations {
    /*
     * begin_sign() - start an RSA sign operation.
     *
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameter needed for the operation.
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    AccumulatingOperationResult begin_sign(
            in HwCryptoKeyMaterial key, in RsaSignParameters parameters);

    /*
     * begin_decrypt() - start an RSA decrypt operation.
     *
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameter needed for the operation.
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    AccumulatingOperationResult begin_decrypt(
            in HwCryptoKeyMaterial key, in RsaSharedSecretParameters parameters);

    /*
     * get_public_key() - Returns the public key portion of @key.
     *
     * @key:
     *      key for which we want to get its public key component
     *
     * Return:
     *      Ok(byte[]) on with the public key on success, specific error code on error.
     */
    // TODO: further define which format is expected for the returned public key
    VectorResult get_public_key(in HwCryptoKeyMaterial key);
}
