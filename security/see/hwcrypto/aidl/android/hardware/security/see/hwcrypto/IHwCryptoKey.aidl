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

import android.hardware.security.see.hwcrypto.IHwCryptoKeyEcOperations;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyGeneration;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyHashOperations;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyProperties;
import android.hardware.security.see.hwcrypto.IHwCryptoKeyRsaOperations;
import android.hardware.security.see.hwcrypto.IHwCryptoKeySymmetricOperations;

interface IHwCryptoKey {
    /*
     * get_key_generation() - Returns an interface with key generation functions
     *
     * Return:
     *      IHwCryptoKeyGeneration on success
     */
    IHwCryptoKeyGeneration get_key_generation();

    /*
     * get_key_properties() - Returns an interface with operations to get and change key attributes
     *
     * Return:
     *      IHwCryptoKeyProperties on success
     */
    IHwCryptoKeyProperties get_key_properties();

    /*
     * get_symmetric_key_operations() - Returns an interface with symmetric key operations functions
     *
     * Return:
     *      IHwCryptoKeySymmetricOperations on success
     */
    IHwCryptoKeySymmetricOperations get_symmetric_key_operations();

    /*
     * get_ec_key_operations() - Returns an interface with EC operations
     *
     * Return:
     *      IHwCryptoKeyEcOperations on success
     */
    IHwCryptoKeyEcOperations get_ec_key_operations();

    /*
     * get_rsa_key_operations() - Returns an interface with RSA operations
     *
     * Return:
     *      IHwCryptoKeyRsaOperations on success
     */
    IHwCryptoKeyRsaOperations get_rsa_key_operations();

    /*
     * get_hash_operations() - Returns an interface with hash operations
     *
     * Return:
     *      IHwCryptoKeyHashOperations on success
     */
    IHwCryptoKeyHashOperations get_hash_operations();
}
