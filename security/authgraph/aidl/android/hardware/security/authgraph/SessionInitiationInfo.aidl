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

/**
 * Session initiation information returned as part of authenticated key exchange.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable SessionInitiationInfo {
    /**
     * An ephemeral EC key created for the ECDH process.
     */
    Key key;

    /**
     * The identity of the party who created the Diffie-Hellman key exchange key.
     */
    Identity identity;

    /**
     * Nonce value specific to this session. The nonce serves three purposes:
     * 1. freshness of key exchange
     * 2. creating a session id (a publicly known value related to the exchanged keys)
     * 3. usage as salt into the HKDF-EXTRACT function during key derivation from the shared DH key
     */
    byte[] nonce;

    /**
     * The protocol version - to prevent protocol downgrade attacks.
     */
    int version;
}
