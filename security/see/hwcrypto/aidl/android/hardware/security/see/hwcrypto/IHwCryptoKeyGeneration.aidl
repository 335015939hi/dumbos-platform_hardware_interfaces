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
import android.hardware.security.see.hwcrypto.types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.types.NullableInt;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;

// TODO: @nullable cannot be used on enums, so using wrappers for them for now

interface IHwCryptoKeyGeneration {
    /*
     * import_clear_key() - Imports a SW clear key into the secure environment.
     *
     * @key_to_be_imported:
     *     key to be imported.
     * @new_key_policy:
     *      Policy of the new key. Defines how the newly created key can be used.
     *
     * Return:
     *      Ok(OpaqueKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    OpaqueKeyMaterial import_clear_key(
            in ExplicitKeyMaterial key_material, in KeyPolicy new_key_policy);

    /*
     * import_wrapped_key() - Imports a previously exported wrapped key.
     *
     * @key_to_be_imported:
     *     key to be imported.
     * @wrapping_key:
     *     key used to original wrap the key to be imported.
     *
     * Return:
     *      Ok(OpaqueKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    OpaqueKeyMaterial import_wrapped_key(
            in byte[] key_to_be_imported, in OpaqueKeyMaterial wrapping_key);

    /*
     * export_wrapped_key() - Exports a wrapped (encrypted) Opaque key.
     *
     * @key_to_be_exported:
     *     key to be exported.
     * @wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * Return:
     *      Ok(byte[]) on success, Err(HAlErrorCode) on error.
     */
    byte[] export_wrapped_key(
            in OpaqueKeyMaterial key_to_be_exported, in OpaqueKeyMaterial wrapping_key);

    /*
     * generate_key() - Generates a new random key.
     *
     * @policy:
     *      Policy of the new key. Defines how the new key can be used and its type.
     * @key_size:
     *     optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
     *     also define the key_size, this parameter is only used for hmac keys. On hmac keys if this
     *     parameter is not specified, <code>generate_key</code> will use an implementation defined
     *     default size.
     *
     * Return:
     *      Ok(OpaqueKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    OpaqueKeyMaterial generate_key(in KeyPolicy policy, in @nullable NullableInt key_size);

    /*
     * derive_key() - Derives a new key.
     *
     * @derivation_key:
     *      Key that will be used to create a new Key. Its type and policy needs to be compatible
     *      with deriving keys out of it.
     * @policy:
     *      Policy of the derived key. Defines how the new key can be used and its type.
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     * @key_size:
     *     optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
     *     also define the key_size, this parameter is only used for hmac keys.
     *
     * Return:
     *      Ok(OpaqueKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    OpaqueKeyMaterial derive_key(in OpaqueKeyMaterial derivation_key, in KeyPolicy policy,
            in byte[] context, in @nullable NullableInt key_size);

    /*
     * secure_key_import() - Securely imports a key using ECDH. This function will generate a shared
     *                       secret using ECDH, derive a shared key using HKDF and then unwrap
     *                       <code>wrapped_key_blob</code> using this key as a AES KWP key.
     *
     * @server_public_key:
     *      Handle to the server public key to be used on the ECDH operation.
     * @client_key:
     *      Private client key to be used on the ECDH operation.
     * @policy:
     *      Optional policy for the created key in case the server do not include this information
     *      on the wrapped key blob.
     * @wrapped_key_blob:
     *      Wrapped key (and optionally a key policy) provided by the server
     * @key_derivation_context:
     *      Context to be used on the HKDF operation
     * @key_derivation_salt:
     *      Salt to be used on the HKDF operation
     */
    OpaqueKeyMaterial secure_key_import(in OpaqueKeyMaterial server_public_key,
            in OpaqueKeyMaterial client_key, in @nullable KeyPolicy policy,
            in byte[] wrapped_key_blob, in byte[] key_derivation_context,
            in byte[] key_derivation_salt);

    /*
     * internal_key_import() - Imports a key from a different service instance. If the key
     *                         don't need to be imported, it will return
     *                         <code>requested_key</code>.
     *
     * @requested_key:
     *      Handle to the key to be imported to the caller service.
     * Return:
     *      A OpaqueKeyMaterial that can be directly be used on the local HWCrypto service on
     *      success.
     */
    OpaqueKeyMaterial internal_key_import(in OpaqueKeyMaterial requested_key);

    /*
     * free_key() - Releases any resources allocated to support the use of
     *              <code>requested_key</code>. After this function is called
     *              <code>requested_key</code> cannot be used anymore.
     *
     */
    void free_key(in OpaqueKeyMaterial key);
}
