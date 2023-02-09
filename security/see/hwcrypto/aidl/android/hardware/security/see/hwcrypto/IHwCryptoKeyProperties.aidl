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

import android.hardware.security.see.hwcrypto.types.HalErrorCode;
import android.hardware.security.see.hwcrypto.types.KeyCharacteristicsResult;
import android.hardware.security.see.hwcrypto.types.KeyDomain;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;

interface IHwCryptoKeyProperties {
    /*
     * set_key_validity() - Sets the period of time the given key should be valid.
     *
     * @key:
     *      key for which we want to set its validity.
     * @validity_period:
     *      how long should the key be valid in seconds.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode set_key_validity(in OpaqueKeyMaterial key, long validity_period);

    KeyCharacteristicsResult get_key_characteristics(in OpaqueKeyMaterial key);

    /*
     * set_impersonation_read_domains() - Sets domains that can use this key to perform operations
     *                                    that require reading a memory buffer owned by this domain.
     *
     * @key:
     *      key for which we want to set its read impersonation domains.
     * @domains:
     *      read impersonation domains.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode set_impersonation_read_domains(in OpaqueKeyMaterial key, in KeyDomain[] domains);

    /*
     * set_impersonation_read_domains() - Sets domains that can use this key to perform operations
     *                                    that require writing a memory buffer owned by this domain.
     *
     * @key:
     *      key for which we want to set its write impersonation domains.
     * @domains:
     *      write impersonation domains.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    HalErrorCode set_impersonation_write_domains(in OpaqueKeyMaterial key, in KeyDomain[] domains);
}
