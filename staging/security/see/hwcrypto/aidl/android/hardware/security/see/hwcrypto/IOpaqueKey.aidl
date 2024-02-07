/*
 * Copyright 2024 The Android Open Source Project
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

import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.OperationType;

interface IOpaqueKey {
    /*
     * exportWrappedKey() - Exports this key as a wrapped (encrypted) blob.
     *
     * @wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * Return:
     *      Wrapped key blob as a byte array on success. Format of the blob is opaque to the service
     *      but has to match the command accepted by
     *      <code>IHwCryptoKeyGeneration::importWrappedKey</code>, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    byte[] exportWrappedKey(in IOpaqueKey wrappingKey);

    /*
     * setKeyValidity() - Sets the period of time this key should be valid. This parameter is not
     *                    part of the policy because Key policies are fixed on creation and this
     *                    parameter can be modified after the key has been created.
     *
     * @validity_period:
     *      how long should the key be valid in seconds.
     *
     * Return:
     *      Nothing on success, service specific error based on <code>HalErrorCode</code> otherwise.
     */
    void setKeyValidity(long validityPeriodSeconds);

    /*
     * getKeyPolicy() - Returns the key policy.
     *
     * Return:
     *      A <code>KeyPolicy</code> on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    KeyPolicy getKeyPolicy();

    /*
     * calculateSharedSecret() - perform an ECDH operation to calculate a shared secret. This
     *                           operation is only valid on EC keys.
     *
     * @publicKey:
     *      Public key of the other party to calculate shared secret. Format of the public key
     *      matches what is accepted by KeyMint.
     *
     * Return:
     *      shared_secret as a byte array on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    byte[] calculateSharedSecret(in byte[] publicKey);

    /*
     * getPublicKey() - Returns the public key portion of this OpaqueKey. This operation is only
     *                  valid for asymmetric keys
     *
     * Return:
     *      public key as a byte array on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise. Format of the public key matches what is accepted
     *      by KeyMint.
     */
    byte[] getPublicKey();
}
