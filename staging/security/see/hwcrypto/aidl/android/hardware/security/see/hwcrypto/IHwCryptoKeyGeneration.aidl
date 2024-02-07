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

import android.hardware.security.see.hwcrypto.IOpaqueKey;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.types.NullableInt;

interface IHwCryptoKeyGeneration {
    /*
     * importClearKey() - Imports a SW clear key into the secure environment.
     *
     * @keyMaterial:
     *     key to be imported.
     * @newKeyPolicy:
     *      Policy of the new key. Defines how the newly created key can be used. Because any
     *      clear key imported into the system is considered to have a
     *      <code>KeyLifetime::PORTABLE</code> lifetime, a call to this function will return an
     *      error if <code>newKeyPolicy.newKeyPolicy</code> is not set to portable.
     *
     * Return:
     *      IOpaqueKey on success, service specific error based on <code>HalErrorCode</code>
     *      otherwise.
     */
    IOpaqueKey importClearKey(in ExplicitKeyMaterial keyMaterial, in KeyPolicy newKeyPolicy);

    /*
     * importWrappedKey() - Imports a previously exported opaque wrapped key.
     *
     * @keyToBeImported:
     *     key to be imported. Format is opaque to the API and implementation defined but has to
     *     match the format returned by a <code>IOpaqueKey::exportWrappedKey</code> call.
     * @wrappingKey:
     *     key used to wrap the original key to be imported.
     *
     * Return:
     *      IOpaqueKey on success, service specific error based on <code>HalErrorCode</code>
     *      otherwise.
     */
    IOpaqueKey importWrappedKey(in byte[] keyToBeImported, in IOpaqueKey wrappingKey);

    /*
     * generate_key() - Generates a new random key.
     *
     * @policy:
     *     Policy of the new key. Defines how the new key can be used and it also encodes the key
     *     type.
     * @key_size:
     *     optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
     *     also define the key_size, this parameter is only used for hmac keys. On hmac keys if this
     *     parameter is not specified, <code>generate_key</code> will use an implementation defined
     *     default size.
     *
     * Return:
     *      IOpaqueKey on success, service specific error based on <code>HalErrorCode</code>
     *      otherwise.
     */
    IOpaqueKey generateKey(in KeyPolicy policy, in @nullable NullableInt keySize);

    /*
     * derive_key() - Derives a new key.
     *
     * @derivation_key:
     *      Key that will be used to create a new Key. Its type and policy needs to be compatible
     *      with deriving keys out of it.
     * @policy:
     *      Policy of the derived key. Defines how the new key can be used and its type.
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation as context. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     * @key_size:
     *     optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
     *     also define the key_size, this parameter is only used for hmac keys.
     *
     * Return:
     *      IOpaqueKey on success, error code service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    IOpaqueKey deriveKey(in IOpaqueKey derivationKey, in KeyPolicy policy, in byte[] context,
            in @nullable NullableInt keySize);
}
