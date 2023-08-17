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

/*
 * Higher level interface to access device-specific keys. It shares definitions with the HwCrypto
 * service but it is intended to be implemented separately (although it can coexist in the same
 * application). Keys returned from this service are intended to be used through the HWCrypto
 * service.
 */
interface IHwCryptoDeviceKeyAccess {
    parcelable DerivedKeyParameters {
        /*
         * If true, the derived key will be consistent and shared across the entire
         * family of devices, given the same input (shared key). If false, the derived key will
         * be unique to the particular device it was derived on.
         */
        boolean batchKey = false;

        /*
         * Policy for the newly derived opaque key. Defines how the key can be used and its type. If
         * null, we will derive a clear key instead and pass it back as an array of bytes on
         * <code>HwCryptoKeyMaterial::explicitKey</code>. For clear keys, the parameter keySize is
         * required and will be used to calculate how many bytes should be returned.
         */
        @nullable KeyPolicy keyPolicy;

        /*
         * Policy used to derive keys tied to specific versions. Using this parameter
         * the caller can tie a derived key to a minimum version of itself, so in the future only
         * iself or a more recent version can derive the same key. This parameter is opaque to the
         * caller and it can be encrypted in the case the client doesn't have permission to know the
         * dice chain. In the case it is encrypted, it should also be salted to avoid reusing the
         * same encrypted blob.
         * If a NULL value is passed on this parameter,
         * <code>DerivedKeyResult.dicePolicyForKeyVersion</code> will generate and return a DICE
         * policy corresponding to the current version as part of its result. If an old policy is
         * used, this function will return the most recent policy as part of the result. If the
         * policy is current, NULL will be returned as the policy.
         * When implementing this function, this parameter shall be one of the components fed
         * to the KDF context and it needs to be checked against the caller DICE certificate before
         * being used.
         */
        @nullable byte[] dicePolicyForKeyVersion;

        /*
         * An arbitrary set of bytes incorporated into the key derivation. May have
         * an implementation-specific maximum length, but it is guaranteed to accept
         * at least 32 bytes.
         */
        byte[] context;

        /*
         * Optional parameter indicating the desired key size. Because most <code>KeyTypes</code>
         * also define the key_size, this parameter is only used for hmac keys if the key is opaque.
         * When deriving a clear key this parameter is always required
         */
        @nullable NullableInt keySize;
    }

    /*
     * hwkeyDeriveVersioned() - Derive a versioned, device-specific or batch key from
     *                          provided context.
     *
     * @parameters:
     *      Parameters used for the key derivation. See <code>DerivedKeyParameters</code> on this
     *      file for more information.
     *
     * Return:
     *      Ok(DerivedKeyResult) on success, service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     */
    DerivedKeyResult hwkeyDeriveVersioned(in DerivedKeyParameters parameters);
}
