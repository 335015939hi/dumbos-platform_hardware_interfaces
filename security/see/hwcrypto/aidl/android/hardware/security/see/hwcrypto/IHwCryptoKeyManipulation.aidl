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
