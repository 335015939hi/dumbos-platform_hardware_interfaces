package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.base_types.HalErrorCode;
import android.hardware.security.see.hwcrypto.base_types.U64Result;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

interface IHwCryptoRng {
    /*
     * add_entropy() - Adds entropy to the random number generator.
     *
     * @entropy_bytes:
     *      Random bytes to be used to add more entropy
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode add_entropy(in byte[] entropy_bytes);
    /*
     * get_secure_rand_bytes() - Retrieves a set of random bytes. These comes from a PRNG function
     *                           seeded using a HW input.
     *
     * @number_bytes:
     *      Number of desired random bytes.
     *
     * Return:
     *      Ok(Byte Vector) on success containing requested random bytes, Err(HAlErrorCode) on
     *      error. Possible error codes include:
     *          - BAD_STATE: If there is not enough entropy on the system
     */
    VectorResult get_secure_rand_bytes(in int number_bytes);
    /*
     * get_secure_rand_number() - Function used to generate a 64-bit random number.
     *
     * Return:
     *      Ok(long) containing a 64 bit random number on success, Err(HAlErrorCode) on error.
     *      Possible error codes include:
     *          - BAD_STATE: If there is not enough entropy on the system
     */
    U64Result get_secure_rand_number();
    /*
     * get_hw_rand_bytes() - Retrieves a set of random bytes generated using Hardware.
     *
     * @number_bytes:
     *      Number of desired random bytes.
     *
     * Return:
     *      Ok(Byte Vector) on success containing requested random bytes, Err(HAlErrorCode) on
     *      error. Possible error codes include:
     *          - BAD_STATE: If there is not enough entropy on the system
     */
    VectorResult get_hw_rand_bytes(in int number_bytes);
}
