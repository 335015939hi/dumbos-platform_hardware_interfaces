/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.AuthenticatedBinding;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.SessionId;

/**
 * The return type of the methods used for ECDH based key exchange. Authgraph uses both
 * authenticated key exchange and non-authenticated key exchange. Some of the fields in this struct
 * are optional when using non-authenticated key exchange.
 *
 * Authenticated key exchange is used for channel establishment.
 *
 * Non authenticated key exchange is used for deriving a key wrapping key from the public key of the
 * wrapping key pair, when the symmetric auth key is not available, which is required when creating
 * auth-bound keys when the user is not authenticated.
 */
@VintfStability
parcelable KEResult {
    /* An ECDH key pair used for key derivation. */
    @nullable Key dh_key;

    /**
     * The arc that encrypts the derived symmetric encryption key from the party's perboot key.
     * Let the two Diffie-Hellman exponentials be: g^x and g^y.
     * A symmetric encryption key is derived from the Diffie-Hellman key agreement as follows:
     * K_seed = HKDF-EXTRACT(g^xy, )
     */
    @nullable Arc shared_key;

    @nullable SessionId session_id;

    @nullable AuthenticatedBinding auth_sign_mac;
    /**
     * The identity of the party who creates the key. This is needed to be set only when creating
     * ephemeral ECDH keys for channel establishment.
     */
    @nullable byte[] identity;

    /**
     * The nonce serves three purposes:
     * 1. freshness of channel establishment
     * 2. creting a session id
     * 3. usage as salt into the HKDF-EXTRACT function during key derivation from the shared DH key
     */
    @nullable byte[] nonce;
}
