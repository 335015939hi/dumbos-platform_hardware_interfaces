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
import android.hardware.security.authgraph.KEResult;
import android.hardware.security.authgraph.KESignature;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.PubKey;

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
     * Creates an ephermeral ECDH key pair and a nonce (of 16 bytes).
     *
     * @return: KEResult including the `Key` containing the public key of the created key pair and
     * an arc from the per-boot key to the private key, the nonce and the persistent identity.
     */
    KEResult create();

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps:
     *     1. create an ephemeral ECDH key pair
     *     2. create a nonce (of 16 bytes)
     *     3. compute the diffie-hellman shared secret: Z
     *     4. compute a salt = bstr .cbor [
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from step #1
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from input `peerDHKey`
     *            sink_nonce:        bstr .size 16,          ; from step #2
     *            source_nonce:      bstr .size 16,          ; from input `peerNonce`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *        ]
     *     5. extract a cryptographic secret S from Z, using the salt from #4 above
     *     6. derive two symmetric encryption keys (for two-way communication) with:
     *        i. b"KE_FIRST_ENCRYPTION_KEY" as context for the key used to encrypt incoming messages
     *       ii.b"KE_SECOND_ENCRYPTION_KEY" as context for the key used to encrypt outgoing messages
     *       Create an arc from the per-boot key to each of the shared keys (a.k.a shared key arcs).
     *     7. mark the shared key arcs as authentication_complete = false in their protected headers
     *     8. derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     9. compute a session_id = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ], and then compute a 256 bits HMAC over the session id with the key from step #8
     *     10.create a signature over the the output of step #9
     *
     * @param peerDHKey - the public key of the ECDH key pair created by the peer (P1)
     *
     * @param peerId - the persistent identity of the peer
     *
     * @param peerNonce - nonce created by the peer
     *
     * @return KEResult including the `Key` containing the public key of the created key pair, the
     * nonce, the persistent identity, two shared key arcs from step #5, session id and the
     * signature over the session id.
     */
    KEResult init(in PubKey peerDHKey, in Identity peerId, in byte[] peerNonce);

    /**
     * This method is invoked on P1 (source).
     * Perform the following steps:
     *     1. compute the diffie-hellman shared secret: Z
     *     2. compute a salt = bstr .cbor [
     *            sink_pub_key:      bstr .cbor PlainPubKey, ; from input `peerDHKey`
     *            source_pub_key:    bstr .cbor PlainPubKey, ; from the output of `create`
     *            sink_nonce:        bstr .size 16,          ; from input `peerNonce`
     *            source_nonce:      bstr .size 16,          ; from the output of `create`
     *            sink_cert_chain:   bstr .cbor ExplicitKeyDiceCertChain, ; from input `peerId`
     *            source_cert_chain: bstr .cbor ExplicitKeyDiceCertChain, ; from own identity
     *        ]
     *     3. extract a cryptographic secret S from Z, using the salt from #2 above
     *     4. derive two symmetric encryption keys (for two-way communication) with:
     *        i. b"KE_FIRST_ENCRYPTION_KEY" as context for the key used to encrypt outgoing messages
     *       ii.b"KE_SECOND_ENCRYPTION_KEY" as context for the key used to encrypt incoming messages
     *       Create an arc from the per-boot key to each of the shared keys (a.k.a shared key arcs).
     *     5. derive a MAC key with b"KE_HMAC_KEY" as the context.
     *     6. compute a session_id = bstr .cbor [
     *            sink_nonce:     bstr .size 16,
     *            source_nonce:   bstr .size 16,
     *        ], and then compute a 256 bits HMAC over the session id with the key from step #5
     *     7. verify the peer's signature over the output of step #6
     *     8. mark the shared key arcs as authentication_complete = true in their protected headers
     *     9. create own signature over the output of step #6
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
     * @return KEResult including the two shared key arcs from step #3, session id and the
     * signature over the session id.
     */
    KEResult finish(in PubKey peerDHKey, in Identity peerId, in KESignature peerSignature,
            in byte[] peerNonce, in Key ownDHKey);

    /**
     * This method is invoked on P2 (sink).
     * Perform the following steps:
     *   1. verify that both shared key arcs have the same session id and peer identity
     *   2. verify the peer's signature over the session id attached to the shared key arcs' headers
     *   3. mark authentication_complete = true in the shared key arcs' headers
     *
     * @param peerSignature - the signature created by the peer over the session id computed by the
     *                        peer
     *
     * @param sharedKeys - two shared key arcs created by P2 in `init`. P2 obtains from the arcs'
     *                     protected headers, the session id and the peer's identity to verify the
     *                     peer's signature over the session id.
     *
     * @param KEResult including the updated shared key arcs
     */
    KEResult authenticationComplete(in KESignature peerSignature, in Arc[] sharedKeys);
}
