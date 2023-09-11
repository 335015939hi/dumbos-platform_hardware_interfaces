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
 * Session initiation information returned as part of Diffie-Hellman based authenticated key
 * exchange.
 */
@VintfStability
parcelable SessionInitiationInfo {
    /**
     * An ephemeral EC key created for the ECDH process.
     */
    Key dh_key;

    /**
     * The identity of the party who created the Diffie-Hellman key agreement key.
     */
    Identity identity;

    /**
     * Nonce value specific to this session. The nonce serves three purposes:
     * 1. freshness of channel establishment
     * 2. creating a session id
     * 3. usage as salt into the HKDF-EXTRACT function during key derivation from the shared DH key
     */
    byte[] nonce;
}
