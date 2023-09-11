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
import android.hardware.security.authgraph.KEAuthCompleteResult;
import android.hardware.security.authgraph.KEInitResult;
import android.hardware.security.authgraph.KESignature;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.PubKey;
import android.hardware.security.authgraph.SessionInfo;
import android.hardware.security.authgraph.SessionInitiationInfo;

/**
 * AuthGraph interface definition for authenticated key exchange between two parties: P1 (source)
 * and P2 (sink).
 * Pre-requisites: each participant should have a:
 *     1. Persistent identity - e.g. a signing key pair with a self signed certificate or a DICE
 *                              certificate chain.
 *     2. A symmetric encryption key kept in memory with per-boot life time (a.k.a per-boot key)
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphKeyExchange {
    /**
     * This method is invoked on P1 (source).
     * Creates an ephemeral ECDH key pair and a nonce (of 16 bytes).
     *
     * @return: SessionInitiationInfo including the `Key` containing the public key of the created
     * key pair and an arc from the per-boot key to the private key, the nonce and the persistent
     * identity.
     */
    SessionInitiationInfo create();

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps (TODO: add more details to each step):
     *     1. create an ephemeral ECDH key pair
     *     2. create a nonce
     *     3. compute the diffie-hellman shared secret: Z
     *     4. derive a cryptographic secret S from Z
     *     5. derive two symmetric encryption keys (for two-way communication) and create an arc
     *        from the per-boot key to each of the shared key (a.k.a shared key arcs)
     *     6. mark the shared key arcs as authentication_complete = false in their protected headers
     *     7. derive a MAC key
     *     8. compute the session id by concatenating the two nonces and computing HMAC over the
     *        session id with the key from step #6 above
     *     9. create a signature over the session id
     *
     * @param peerDHKey - the public key of the ECDH key pair created by the peer (P1)
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerNonce - nonce created by the peer
     *
     * @return KEInitResult including the `Key` containing the public key of the created key pair,
     * the nonce, the persistent identity, two shared key arcs from step #5, session id and the
     * signature over the session id.
     */
    KEInitResult init(in PubKey peerDHKey, in Identity peerId, in byte[] peerNonce);

    /**
     * This method is invoked on P1 (source).
     * Perform the following steps (TODO: add more details to each step):
     *     1. compute the diffie-hellman shared secret: Z
     *     2. derive a cryptographic secret S from Z
     *     3. derive two symmetric encryption keys (for two-way communication) and create an arc
     *        from the per-boot key to each of the shared key (a.k.a shared key arcs)
     *     4. derive a MAC key
     *     5. compute the session id by concatenating the two nonces and computing HMAC over the
     *        session id with the key from step #4 above
     *     6. verify the peer's signature over the session id computed in step #5 above
     *     7. mark the shared key arcs as authentication_complete = true in their protected headers
     *     8. create own signature over the session id
     *
     * @param peerDHKey - the public key of the ECDH key pair created by the peer (P2)
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer
     *
     * @param peerNonce - nonce created by the peer
     *
     * @param ownDHKey - the key created by P1 (source) in `create()` for key agreement
     *
     * @return SessionInfo including the two shared key arcs from step #3, session id and the
     * signature over the session id.
     */
    SessionInfo finish(in PubKey peerDHKey, in Identity peerId, in KESignature peerSignature,
            in byte[] peerNonce, in Key ownDHKey);

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps:
     *     1. verify that both shared key arcs have the same session id and peer id
     *     2. verify the signature over the session id attached to the session key arcs
     *     3. mark the session key arcs as authentication_complete = true in their protected headers
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer
     *
     * @param sharedKeys - two shared key arcs created by P2 in `init`. P2 obtains the computed
     *                     session id and the peer's identity to verify peer's signature over the
     *                     session id, from the information attached to the arcs' protected headers.
     *
     * @param KEAuthCompleteResult including the updated shared key arcs
     */
    KEAuthCompleteResult authenticationComplete(in KESignature peerSignature, in Arc[] sharedKeys);
}
