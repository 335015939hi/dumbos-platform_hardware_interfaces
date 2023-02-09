package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.base_types.AccumulatingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.EcSignParameters;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

interface IHwCryptoKeyEcOperations {
    /*
     * begin_sign() - start an EC sign operation.
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
            in HwCryptoKeyMaterial key, in EcSignParameters parameters);

    /*
     * begin_shared_secret() - start an ECDH operation to calculate a shared secret.
     *
     * @key:
     *      key to be used on the operation
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    AccumulatingOperationResult begin_shared_secret(in HwCryptoKeyMaterial key);

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
