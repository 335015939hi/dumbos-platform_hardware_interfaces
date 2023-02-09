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
import android.hardware.security.see.hwcrypto.IAeadOperation;
import android.hardware.security.see.hwcrypto.IEmittingOperation;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.ComponentVersion;
import android.hardware.security.see.hwcrypto.types.ExplicitKeyMaterial;
import android.hardware.security.see.hwcrypto.types.KeyMaterialWithInfo;
import android.hardware.security.see.hwcrypto.types.KeyVersionSource;
import android.hardware.security.see.hwcrypto.types.NullableInt;
import android.hardware.security.see.hwcrypto.types.NullableKdfVersion;
import android.hardware.security.see.hwcrypto.types.OpaqueKeyMaterial;
import android.hardware.security.see.hwcrypto.types.SymmetricOperationParameters;

/*
 * Higher level interface to access device-specific keys. It shares definitions with the HwCrypto
 * service but it is intended to be implemented separately (although it can coexist in the same
 * application). Keys returned from this service are inteded to be used through the HWCrypto
 * service.
 */
interface IHwCryptoDeviceKeyAccess {
    /*
     * get_keyslot_data() - Gets the keyslot key material referenced by slot_id.
     *
     * @slot_id:
     *      string identifier for the requested keyslot
     *
     * This interface is used to access key material specified on the HwCrypto spec (TBD). The type
     * of the returned key is also part of the spec. Because the returned key is opaque, it can only
     * be used through the different HwCrypto interfaces. Because the keys live in a global
     * namespace the identity of the caller needs to be checked to verify that it has permission to
     * accesses the requested key.
     *
     * Return:
     *      Ok(OpaqueKeyMaterial) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - IoError: if there's an issue communicating with the service
     *          - NotFound: if keyslot is not found
     */
    OpaqueKeyMaterial get_keyslot_data(String slot_id);

    /*
     * hwkey_derive_versioned() - Derive a versioned, device-specific or batch key from
     *                            provided context.
     *
     * @kdf_version:
     *     version of the KDF to use; can be set to Null to use latest algorithm version.
     * @batch_key:
     *     if true, the derived key will be consistent and shared across the entire
     *     family of devices, given the same input (shared key). If false, the derived key will
     *     be unique to the particular device it was derived on.
     * @key_policy:
     *     Policy of the newly derived key. Defines how the key can be used and its type.
     * @rollback_version_source:
     *     specifies whether the @rollback_versions must have been committed. If
     *     <code>CommittedVersion</code> is specified, the system must guarantee
     *     that software with a lower rollback version cannot ever run on a future
     *     boot. (see &enum KeyVersionSource)
     * @rollback_versions:
     *     (in) the different components (OS, etc.) rollback versions to be incorporated into
     *     the key derivation. Must be less than or equal to the current component rollback
     *     version from @rollback_version_source. If a component version isn't set, the latest
     *     available version will be used.
     * @context:
     *     an arbitrary set of bytes incorporated into the key derivation. May have
     *     an implementation-specific maximum length, but it is guaranteed to accept
     *     at least 32 bytes.
     * @key_size:
     *     optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
     *     also define the key_size, this parameter is only used for hmac keys.
     * @derive_opaque_key:
     *     if true, the returned key material will be opaque; if false it will be in the clear.
     *     Note that each case returns a different key value, so that an opaque key cannot be
     *     re-derived in the clear.
     *
     * Return:
     *      Ok(KeyMaterialWithInfo) on success, Err(HAlErrorCode) on error. Possible error
     *      codes include:
     *          - NotValid - invalid parameters
     *          - NotImplemented - the requested version source or KDF mode is not supported
     */
    KeyMaterialWithInfo hwkey_derive_versioned(in @nullable NullableKdfVersion kdf_version,
            boolean batch_key, in KeyPolicy key_policy, KeyVersionSource rollback_version_source,
            in ComponentVersion[] rollback_versions, in byte[] context,
            in @nullable NullableInt key_size, boolean derive_opaque_key);
}
