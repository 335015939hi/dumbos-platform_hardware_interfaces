package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;
import android.hardware.security.see.hwcrypto.base_types.U64Result;

// TODO: It would be better if we can return some equivalent to Result<(), HalErrorCode> but this
//       doesn't seem to be a valid union for AIDL.

interface HwCryptoRng {
    /*
     * add_entropy() - Adds entropy to the random number generator.
     * @entropyBytes:
     *      Random bytes to be used to add more entropy
     *
     * Return:
     *      Ok on success, error code less than 0 on error.
     */
    HalErrorCode add_entropy(in byte[] entropyBytes);
    /*
     * get_secure_rand_bytes() - Retrieves a set of random bytes. These comes from a PRNG function
     *                           seeded using a HW input.
     * @randomBytes:
     *      Generated random bytes. Function will generate as many random bytes as the size of
     *      @randomBytes
     *
     * Return:
     *      Ok on success, error code less than 0 on error. Possible error codes include:
     *          - BadState: If there is not enough entropy on the system
     */
    HalErrorCode get_secure_rand_bytes(inout byte[] randomBytes);
    /*
     * get_secure_rand_number() - Function used to generate a random number.
     *
     * Return:
     *      Ok on success, error code less than 0 on error. Possible error codes include:
     *          - BadState: If there is not enough entropy on the system
     */
    U64Result get_secure_rand_number();
    /*
     * get_hw_rand_bytes() - Retrieves a set of random bytes generated using Hardware.
     * @randomBytes:
     *      Generated random bytes. Function will generate as many random bytes as the size of
     *      @randomBytes
     *
     * Return:
     *      Ok on success, error code less than 0 on error. Possible error codes include:
     *          - BadState: If there is not enough entropy on the system
     */
    HalErrorCode get_hw_rand_bytes(inout byte[] randomBytes);
}
