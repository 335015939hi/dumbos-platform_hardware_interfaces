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
package android.hardware.security.see.hwcrypto.types;

/*
 * Type that represents an EC key.
 */
union EcKey {
    parcelable NistKey {
        /*
         * DER-encoded EcPrivate key structure (RFC 5915). Follows the same encoding used by
         * BoringSSL EC_KEY_parse_private_key function
         */
        byte[] keyMaterial;
    }

    parcelable Ed25519Key {
        /*
         * Raw EdDsa with curve25529 key material.
         */
        byte[32] keyMaterial;
    }

    parcelable X25519Key {
        /*
         * Raw ECDH key using curve25529.
         */
        byte[32] keyMaterial;
    }

    /*
     * Nist P-224 elliptic curve key.
     */
    NistKey p224;

    /*
     * Nist P-256 elliptic curve key.
     */
    NistKey p256;

    /*
     * Nist P-384 elliptic curve key.
     */
    NistKey p384;

    /*
     * Nist P-521 elliptic curve key.
     */
    NistKey p521;

    /*
     * EdDsa key using a Curve25519.
     */
    Ed25519Key ed25519;

    /*
     * Curve25519 elliptic curve key.
     */
    X25519Key x25519;
}
