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

import android.hardware.security.see.hwcrypto.IAccumulatingOperation;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.types.RsaDecryptParameters;
import android.hardware.security.see.hwcrypto.types.RsaSignParameters;

interface IHwCryptoKeyRsaOperations {
    /*
     * begin_sign() - start an RSA sign operation.
     *
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameter needed for the operation.
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    IAccumulatingOperation begin_sign(in OpaqueKeyMaterial key, in RsaSignParameters parameters);

    /*
     * begin_decrypt() - start an RSA decrypt operation.
     *
     * @key:
     *      key to be used on the operation
     * @parameters:
     *      parameter needed for the operation.
     *
     * Return:
     *      Ok(IAccumulatingOperation) on success, specific error code on error.
     */
    IAccumulatingOperation begin_decrypt(
            in OpaqueKeyMaterial key, in RsaDecryptParameters parameters);

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
    byte[] get_public_key(in OpaqueKeyMaterial key);
}
