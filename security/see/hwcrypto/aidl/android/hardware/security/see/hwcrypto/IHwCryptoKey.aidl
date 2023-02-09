package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.IHwCryptoKeyAsymmetricOperations;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyGeneration;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyManipulation;
import android.hardware.security.see.hwcrypto.IHwCryptoKeySymmetricOperations;

interface IHwCryptoKey {
    /*
     * get_key_generation() - Returns an interface with key generation functions
     *
     * Return:
     *      IHwCryptoKeyGeneration on success
     */
    @nullable IHwCryptoKeyGeneration get_key_generation();

    /*
     * get_key_manipulation() - Returns an interface with operations to change key attributes
     *
     * Return:
     *      IHwCryptoKeyAsymmetricOperations on success
     */
    @nullable IHwCryptoKeyManipulation get_key_manipulation();

    /*
     * get_key_operations() - Returns an interface with symmetric key operations functions
     *
     * Return:
     *      IHwCryptoKeySymmetricOperations on success
     */
    @nullable IHwCryptoKeySymmetricOperations get_symmetric_key_operations();

    /*
     * get_key_operations() - Returns an interface with asymmetric key operations functions
     *
     * Return:
     *      IHwCryptoKeyAsymmetricOperations on success
     */
    @nullable IHwCryptoKeyAsymmetricOperations get_asymmetric_key_operations();
}
