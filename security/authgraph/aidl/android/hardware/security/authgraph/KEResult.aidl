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
 * The return type of the three methods used to do authenticated key exchange via the sigma
 * protocol.
 */
@VintfStability
parcelable KEResult {
    @nullable Key dh_key;
    @nullable AuthenticatedBinding auth_sign_mac;
    @nullable SessionId session_id;
    @nullable Arc shared_key;
    /**
     * The identity of the party who creates the key. This is needed to be set only when creating
     * ephemeral ECDH keys for channel establishment.
     */
    @nullable byte[] identity;

    /**
     * The nonce serves three purposes:
     * 1. freshness of channel establishment
     * 2. creting a session id
     * 3. usage as a salt for key independence during key derivation from the shared DH key
     */
    @nullable byte[] nonce;
}
