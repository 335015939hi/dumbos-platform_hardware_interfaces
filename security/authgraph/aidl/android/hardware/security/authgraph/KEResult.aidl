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
import android.hardware.security.authgraph.Identity;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.Signature;

/**
 * The return type of the methods used for Diffie-Hellman based authenticated key exchange.
 */
@VintfStability
parcelable KEResult {
    /* An ECDH key pair used for key derivation. */
    @nullable Key dh_key;

    /**
     * The arc that encrypts the two derived symmetric encryption keys (for two-way communication)
     * from the party's per-boot key.
     */
    @nullable Arc[] shared_keys;

    /**
     * The value of the session id computed by the two parties during the authenticate key
     * exchange. Apart from the usage of the session id by the two peers, session id is also useful
     * to verify (by a third party) that the key exchange was successful.
     */
    @nullable byte[] session_id;

    /**
     * The signature over the session id by the party who computed the session id.
     */
    @nullable Signature signature;

    /**
     * The identity of the party who creates the key.
     */
    @nullable Identity identity;

    /**
     * The nonce serves three purposes:
     * 1. freshness of channel establishment
     * 2. creting a session id
     * 3. usage as salt into the HKDF-EXTRACT function during key derivation from the shared DH key
     */
    @nullable byte[] nonce;
}
