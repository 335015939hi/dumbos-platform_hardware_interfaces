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
 * This is part of the return type from the authenticated key exchange methods which includes the
 * peers' signature on the peer's Diffie-Hellman exponential and the nonce provided by the peer, and
 * the MAC computed by the peer on the peer's identity using a key derived from the Diffie-Hellman
 * shared secret.
 */
@VintfStability
parcelable AuthenticatedBinding {
    /**
     * TODO: define how this is computed and the data format.
     */
    byte[] encodedAuthenticatedBinding;
}
