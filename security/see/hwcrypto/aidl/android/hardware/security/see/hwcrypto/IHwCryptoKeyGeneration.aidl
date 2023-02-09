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
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.base_types.AeadOperationResult;
import android.hardware.security.see.hwcrypto.base_types.ComponentVersion;
import android.hardware.security.see.hwcrypto.base_types.DmaOperationBuffers;
import android.hardware.security.see.hwcrypto.base_types.EmittingOperationResult;
import android.hardware.security.see.hwcrypto.base_types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.HwCryptoKeyResult;
import android.hardware.security.see.hwcrypto.base_types.KeyType;
import android.hardware.security.see.hwcrypto.base_types.KeyVersionSource;
import android.hardware.security.see.hwcrypto.base_types.NullableKdfVersion;
import android.hardware.security.see.hwcrypto.base_types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.base_types.SymmetricKeyType;
import android.hardware.security.see.hwcrypto.base_types.SymmetricOperationParameters;
import android.hardware.security.see.hwcrypto.base_types.VectorResult;

// TODO: @nullable cannot be used on enums, so using wrappers for them for now

interface IHwCryptoKeyGeneration {
    /*
     * get_keyslot_data() - Gets the keyslot key material referenced by slot_id.
     *
     * @slot_id:
     *      string identifier for the requested keyslot
     *
     * Because this access a shared key, the identity of the caller needs to be checked
     * to verify that it has permission to access the requested key.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - IoError: if there's an issue communicating with the service
     *          - NotFound: if keyslot is not found
     */
    HwCryptoKeyResult get_keyslot_data(String slot_id);

    /*
     * hwkey_derive_versioned() - Derive a versioned, device-specific or batch key from
     *                            provided context.
     *
     * @kdf_version:
     *     version of the KDF to use; can be set to Null to use default value.
     * @batch_key:
     *     if true, the derived key will be consistent and shared across the entire
     *     family of devices, given the same input (shared key). If false, the derived key will
     *     be unique to the particular device it was derived on.
     * @key_type:
     *     desired type of key to be generated (AES, etc.). Can be set to Null to use default value.
     * @rollback_version_source:
     *     specifies whether the @rollback_versions must have been committed. If
     *     %CommittedVersion is specified, the system must guarantee
     *     that software with a lower rollback version cannot ever run on a future
     *     boot. (see &enum KeyVersionSource)
     * @rollback_versions:
     *     (in/out) the different components (OS, etc.) rollback versions to be incorporated into
     *     the key derivation. Must be less than or equal to the current component rollback
     *     version from @rollback_version_source. If a component version isn't set, the latest
     *     available version will be used. After execution, the versions used for the key derivation
     *     will be present on this argument
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - NotValid - invalid parameters
     *          - NotImplemented - the requested version source or KDF mode is not supported
     */
    HwCryptoKeyResult hwkey_derive_versioned(in @nullable NullableKdfVersion kdf_version,
            boolean batch_key, in @nullable KeyType key_type,
            KeyVersionSource rollback_version_source,
            in @nullable ComponentVersion[] rollback_versions, in byte[] context);

    /*
     * import_clear_key() - Imports a SW key into the secure environment.
     *
     * @key_to_be_imported:
     *     key to be imported.
     * @new_key_policy:
     *      Policy of the new key. Defines how the newly created key can be used.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult import_clear_key(
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
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult import_wrapped_key(
            in byte[] key_to_be_imported, in OpaqueKeyMaterial wrapping_key);

    /*
     * export_wrapped_key() - Imports a SW key into the secure environment.
     *
     * @key_to_be_exported:
     *     key to be imported.
     * @wrapping_key:
     *     wrapping key. It needs to be an opaque key and its policy needs to indicate that it can
     *     be used for key wrapping.
     *
     * Return:
     *      Ok(byte[]) on success, Err(HAlErrorCode) on error.
     */
    VectorResult export_wrapped_key(
            in OpaqueKeyMaterial key_to_be_exported, in OpaqueKeyMaterial wrapping_key);

    /*
     * generate_key() - Generates a new key.
     *
     * @policy:
     *      Policy of the new key. Defines how the new key can be used.
     * @key_type:
     *     desired type of key to be generated (AES, etc.).
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult generate_key(in KeyPolicy policy, in KeyType key_type);

    /*
     * derive_key() - Derives a new key.
     *
     * @derivation_key:
     *      Key that will be used to create a new Key. Its type and policy needs to be compatible
     *      with deriving keys out of it.
     * @policy:
     *      Policy of the derived key. Defines how the new key can be used.
     * @key_type:
     *     desired type of key to be generated (AES, etc.).
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     *
     * Return:
     *      Ok(HwCryptoKeyMaterial) on success, Err(HAlErrorCode) on error.
     */
    HwCryptoKeyResult derive_key(in OpaqueKeyMaterial derivation_key, in KeyPolicy policy,
            in KeyType key_type, in byte[] context);

    /*
     * secure_import_key_into_engine() - Creates a secure channel with a server to import a key.
     *                            (TODO: complete definition).
     */
}
