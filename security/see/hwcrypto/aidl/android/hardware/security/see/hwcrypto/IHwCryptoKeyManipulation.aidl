package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;

interface IHwCryptoKeyManipulation {
    /*
     * set_key_validity() - Returns an interface with key generation functions
     *
     * @key:
     *      key for which we want to set its validity.
     * @validity_period:
     *      how long should the key be valid (units TBD)
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode set_key_validity(in HwCryptoKeyMaterial key, long validity_period);
}
