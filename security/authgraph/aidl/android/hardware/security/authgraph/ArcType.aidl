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

/**
 * Arc type indicates which key is encrypted in an Arc as the payload key. Arc type is used in
 * the primitive operations to decide which variants of the enum types are expected/returned and
 * which permissions/limitations are attached to the arcs.
 */
@VintfStability
union ArcType {
    /*An ephemeral key pair created to be used in the key exchange protocol*/
    int EPHEMERAL_KE_KEY = 1;

    /*The secret key shared between two domains at the end of channel establishment*/
    int CHANNEL_KEY = 2;

    /**
     * The key derived from the hardware backed master key of the source domain* to be used in
     * Authgraph
     */
    int SOURCE_KEY = 3;

    /*The key created by the source domain to protect a user's resource held at a sink domain*/
    int AUTH_KEY = 4;

    /**
     * The key pair created by the sink domain to protect a user's resource with an auth key from
     * a particular source domain, when such auth key is not available at the time of the creation
     * of the resource.
     */
    int WRAPPING_KEY = 5;

    /**
     * A key created by the sink domain per each user's resource, as an indirection to protect the
     * resource with an auth key from a particular source domain
     */
    int KEY_WRAPPING_KEY = 6;

    /**
     * The cihper text returned by the encap() method of KEM used during the key exchange protocol.
     */
    int KEM_CIPHER_TEXT = 7;
}
