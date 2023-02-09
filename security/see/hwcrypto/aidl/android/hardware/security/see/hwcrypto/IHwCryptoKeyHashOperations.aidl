package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.AccumulatingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.HmacOperationParameters;

interface IHwCryptoKeyHashOperations {
    /*
     * begin_hmac() - start an hmac cryptographic operation.
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameters that specify the desired cryptographic operation. Should match the provided
     *      key.
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    AccumulatingOperationResult begin_hmac(
            in HwCryptoKeyMaterial key, in HmacOperationParameters parameters);
}
