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
    void add_entropy(in byte[] entropy_bytes);
    /*
     * get_secure_rand_bytes() - Retrieves a set of random bytes. These comes from a PRNG function
     *                           seeded using a HW input.
     *
     * @number_bytes:
     *      Number of desired random bytes.
     *
     * Return:
     *      Ok(Byte Vector) on success containing requested random bytes, specific error code on
     *      error.
     */
    byte[] get_secure_rand_bytes(in int number_bytes);
    /*
     * get_secure_rand_number() - Function used to generate a 64-bit random number.
     *
     * Return:
     *      Ok(long) containing a 64 bit random number on success, specific error code on error.
     */
    long get_secure_rand_number();
    /*
     * get_hw_rand_bytes() - Retrieves a set of random bytes generated using Hardware.
     *
     * @number_bytes:
     *      Number of desired random bytes.
     *
     * Return:
     *      Ok(Byte Vector) on success containing requested random bytes, specific error code on
     *      error.
     */
    byte[] get_hw_rand_bytes(in int number_bytes);
}
