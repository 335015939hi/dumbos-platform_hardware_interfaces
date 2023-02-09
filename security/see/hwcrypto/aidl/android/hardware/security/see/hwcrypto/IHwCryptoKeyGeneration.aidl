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

import android.hardware.security.see.hwcrypto.IOpaqueKey;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.types.NullableInt;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;

interface IHwCryptoKeyGeneration {
    /*
     * import_clear_key() - Imports a SW clear key into the secure environment.
     *
     * @key_material:
     *     key to be imported.
     * @new_key_policy:
     *      Policy of the new key. Defines how the newly created key can be used.
     *
     * Return:
     *      Ok(IOpaqueKey) on success, error code otherwise.
     */
    IOpaqueKey import_clear_key(in ExplicitKeyMaterial key_material, in KeyPolicy new_key_policy);

    /*
     * import_wrapped_key() - Imports a previously exported opaque wrapped key.
     *
     * @key_to_be_imported:
     *     key to be imported.
     * @wrapping_key:
     *     key used to original wrap the key to be imported.
     *
     * Return:
     *      Ok(IOpaqueKey) on success, , error code otherwise.
     */
    IOpaqueKey import_wrapped_key(in byte[] key_to_be_imported, in IOpaqueKey wrapping_key);

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
     *      Ok(IOpaqueKey) on success, error code otherwise.
     */
    IOpaqueKey generate_key(in KeyPolicy policy, in @nullable NullableInt key_size);

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
     *      Ok(IOpaqueKey) on success, error code otherwise.
     */
    IOpaqueKey derive_key(in IOpaqueKey derivation_key, in KeyPolicy policy, in byte[] context,
            in @nullable NullableInt key_size);

    /*
     * internal_key_import() - Imports a key from a different service instance.
     *
     * @requested_key:
     *      Handle to the key to be imported to the caller service.
     * Return:
     *      A IOpaqueKey that can be directly be used on the local HWCrypto service on
     *      success.
     */
    IOpaqueKey internal_key_import(in OpaqueKeyMaterial requested_key);
}
