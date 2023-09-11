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
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.PubKey;
import android.hardware.security.authgraph.SessionIdSignature;
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
     * Create an ephermeral EC key pair on NIST curve P-256 and a nonce (of 16 bytes) for
     * key agreement.
     *
     * @return: SessionInitiationInfo including the `Key` containing the public key of the created
     * key pair and an arc from the per-boot key to the private key, the nonce, the persistent
     * identity and the latest protocol version supported.
     */
    SessionInitiationInfo create();

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps for key agreement:
     *     1. Create an ephemeral EC key pair on NIST curve P-256.
     *     2. Create a nonce (of 16 bytes).
     *     3. Compute the diffie-hellman shared secret: Z.
     *     4. Compute a salt = bstr .cbor [
     *            source_version:    int,                    ; from input `peerVersion`
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from step #1
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from input `peerPubKey`
     *            sink_nonce:        bstr .size 16,          ; from step #2
     *            source_nonce:      bstr .size 16,          ; from input `peerNonce`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *        ]
     *     5. Extract a cryptographic secret S from Z, using the salt from #4 above.
     *     6. Derive two symmetric encryption keys of 256 bits with:
     *        i. b"KE_FIRST_ENCRYPTION_KEY" as context for the key used to encrypt incoming messages
     *       ii.b"KE_SECOND_ENCRYPTION_KEY" as context for the key used to encrypt outgoing messages
     *     7. Create arcs from the per-boot key to each of the two shared keys from step #6 and
     *        mark authentication_complete = false in arcs' protected headers.
     *     8. Derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     9. Compute session_id_input = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ],
     *     10.Compute a session_id as a 256 bits HMAC over the session_id_input from step#9 with
     *        the key from step #8.
     *     11.Create a signature over the session_id from step #10, using the signing key which is
     *        part of the party's identity.
     *
     * @param peerPubKey - the public key of the key pair created by the peer (P1) for key agreement
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerNonce - nonce created by the peer
     *
     * @param peerVersion - latest version of the protocol supported by the peer
     *
     * @return KEInitResult including the `Key` containing the public key of the created key pair,
     * the nonce, the persistent identity, two shared key arcs from step #7, session id, signature
     * over the session id and the negotiated protocol version.
     */
    KEInitResult init(
            in PubKey peerPubKey, in Identity peerId, in byte[] peerNonce, in int peerVersion);

    /**
     * This method is invoked on P1 (source).
     * Perform the following steps:
     *     1. Compute the diffie-hellman shared secret: Z.
     *     2. Compute a salt = bstr .cbor [
     *            source_version:    int,                    ; the protocol version used in `create`
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from input `peerPubKey`
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from the output of `create`
     *            sink_nonce:        bstr .size 16,          ; from input `peerNonce`
     *            source_nonce:      bstr .size 16,          ; from the output of `create`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *        ]
     *     3. Extract a cryptographic secret S from Z, using the salt from #2 above.
     *     4. Derive two symmetric encryption keys of 256 bits with:
     *        i. b"KE_FIRST_ENCRYPTION_KEY" as context for the key used to encrypt outgoing messages
     *       ii.b"KE_SECOND_ENCRYPTION_KEY" as context for the key used to encrypt incoming messages
     *     5. Derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     6. Compute session_id_input = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ],
     *     7. Compute a session_id as a 256 bits HMAC over the session_id_input from step #6 with
     *        the key from step #5.
     *     8. Verify the peer's signature over the session_id from step #7. If successful, proceed.
     *     9. Create arcs from the per-boot key to each of the two shared keys from step #4 and
     *        mark authentication_complete = true in arcs' protected headers.
     *     10.Create a signature over the session_id from step #7, using the signing key which is
     *        part of the party's identity.
     *
     * @param peerPubKey - the public key of the key pair created by the peer (P2) for key agreement
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer
     *
     * @param peerNonce - nonce created by the peer
     *
     * @param peerVersion - the protocol version negotiated with the peer
     *
     * @param ownKey - the key created by P1 (source) in `create()` for key agreement
     *
     * @return SessionInfo including the two shared key arcs from step #9, session id and the
     * signature over the session id.
     */
    SessionInfo finish(in PubKey peerPubKey, in Identity peerId,
            in SessionIdSignature peerSignature, in byte[] peerNonce, in int peerVersion,
            in Key ownKey);

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps:
     *   1. Verify that both shared key arcs have the same session id and peer identity.
     *   2. Verify the peer's signature over the session id attached to the shared key arcs'
     *      headers. If successful, proceed.
     *   3. Mark authentication_complete = true in the shared key arcs' headers
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer
     *
     * @param sharedKeys - two shared key arcs created by P2 in `init`. P2 obtains from the arcs'
     *                     protected headers, the session id and the peer's identity to verify the
     *                     peer's signature over the session id.
     *
     * @param KEAuthCompleteResult including the updated shared key arcs
     */
    KEAuthCompleteResult authenticationComplete(
            in SessionIdSignature peerSignature, in Arc[] sharedKeys);
}
