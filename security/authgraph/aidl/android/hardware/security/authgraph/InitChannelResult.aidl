/*
 * Copyright (C) 2021 The Android Open Source Project
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
 * This is the result returned by the IAuthGraphCommon initChannel() function.
 * @hide
 */
@VintfStability
parcelable InitChannelResult {
    /**
     * Public key of the ephemeral elliptic curve (EC) key generated to establish channel keys,
     * which is signed by the persistent signing key of the domain.
     */
    byte[] signedPublicKey;

    /**
     * A random nonce to be encrypted by the other party, using their outgoing channel key.
     * This is to verify that the established channel keys are usable.
     */
    byte[] nonceToBeEncrypted;

    /**
     * Optional arc from per-boot-key to the private key of the ephemeral EC key, if the domain
     * does not want to maintain states across the calls for channel establishment. Format is the
     * same as the output of AES-GCM encryption operation.
     */
    @nullable byte[] arcFromPerBootKeyToPrivateKey;
}
