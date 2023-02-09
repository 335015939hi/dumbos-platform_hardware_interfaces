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

import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.KeyDomain;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.types.OperationType;
import android.hardware.security.see.hwcrypto.types.ProtectionId;

interface IOpaqueKey {
    /*
     * get_shareable_token() - Returns a token that can shared with another HWCrypto client.
     *
     * @entropy_bytes:
     *      Random bytes to be used to add more entropy
     *
     * Return:
     *      <code>OpaqueKeyMaterial</code> token on success.
     */
    OpaqueKeyMaterial get_shareable_token();

    /*
     * export_wrapped_key() - Exports this key as a wrapped (encrypted) blob.
     *
     * @wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * Return:
     *      Wrapped key blob as a byte array on success.
     */
    byte[] export_wrapped_key(in IOpaqueKey wrapping_key);

    /*
     * set_key_validity() - Sets the period of time this key should be valid.
     *
     * @validity_period:
     *      how long should the key be valid in seconds.
     *
     * Return:
     *      Nothing on success, specific error code on error.
     */
    void set_key_validity(long validity_period);

    /*
     * get_key_policy() - Returns the key policy.
     *
     * Return:
     *      A <code>KeyPolicy</code> on success.
     */
    KeyPolicy get_key_policy();

    /*
     * set_protection_id() - Sets domains that can use this key to perform operations
     *                       that require reading a memory buffer owned by this domain.
     *
     * @protection_id:
     *      ID of the given use case to provide protection for. The way of protect the buffer will
     *      be platform dependent.
     * @allowed_operations:
     *      array of allowed operations. Allowed operations are either READ or WRITE.
     *
     * Return:
     *      NO_ERROR on success, specific error code on error.
     */
    void set_protection_id(in ProtectionId protection_id, in OperationType[] allowed_operations);

    /*
     * calculate_shared_secret() - perform an ECDH operation to calculate a shared secret. This
     *                             operation is only valid on EC keys.
     *
     * @public_key:
     *      Public key of the other party to calculate shared secret. Format of the public key
     *      matches what is accepted by KeyMint.
     *
     * Return:
     *      Ok(byte[] shared_secret) on success, specific error code on error.
     */
    byte[] calculate_shared_secret(in byte[] public_key);

    /*
     * get_public_key() - Returns the public key portion of this OpaqueKey. This operation is only
     *                    valid for asymmetric keys
     *
     * Return:
     *      Ok(byte[]) on with the public key on success, specific error code on error. Format of
     *      the public key matches what is accepted by KeyMint.
     */
    byte[] get_public_key();
}
