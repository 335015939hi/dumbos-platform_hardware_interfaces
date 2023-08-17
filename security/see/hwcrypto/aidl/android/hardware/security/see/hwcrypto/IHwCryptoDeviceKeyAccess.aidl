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

import android.hardware.security.see.hwcrypto.DerivedKeyResult;
import android.hardware.security.see.hwcrypto.IOpaqueKey;
import android.hardware.security.see.hwcrypto.KeyPolicy;
import android.hardware.security.see.hwcrypto.types.NullableInt;
import android.hardware.security.see.hwcrypto.types.NullableKdfVersion;

/*
 * Higher level interface to access device-specific keys. It shares definitions with the HwCrypto
 * service but it is intended to be implemented separately (although it can coexist in the same
 * application). Keys returned from this service are intended to be used through the HWCrypto
 * service.
 */
interface IHwCryptoDeviceKeyAccess {
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
     * @versioned_dice_policy:
     *     policy used to be able to derive keys tied to specific versions. Using this parameter
     *     the caller can tie a derived key to a minimum version of itself, so in the future only
     *     iself or a more recent version can derive the same key. This parameter is opaque to the
     *     caller and it can be encrypted in the case the client doesn't have permission to know the
     *     dice chain. In the case it is encrypted, it should also be salted to prevent to receive
     *     always the same encrypted blob.
     *     If a NULL value is passed to this function, a DICE policy corresponding to the current
     *     version will be generated and returned as part of a result. If an old policy is used,
     *     this function will return the most recent policy as part of the result, but if the policy
     *     is current, a NULL will be returned as the policy  on the return value.
     *     When implementing this function, this parameter shall be one of the components fed
     *     to the KDF context and it needs to be checked against the caller DICE certificate before
     *     being used.
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
     *      Ok(DerivedKeyResult) on success, error code otherwise.
     */
    DerivedKeyResult hwkey_derive_versioned(in @nullable NullableKdfVersion kdf_version,
            boolean batch_key, in KeyPolicy key_policy,
            in @nullable byte[] versioned_dice_policy, in byte[] context,
            in @nullable NullableInt key_size, boolean derive_opaque_key);
}
