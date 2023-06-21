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
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.PubKey;
import android.hardware.security.authgraph.Signature;

/**
 * Authgraph interface definition for authenticated key exchange.
 * TODO: add pre-requisites.
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphKeyExchange {
    /**
     * Creates a key and returns an arc from the per-boot key to the secret key. If the created key
     * is an asymmetric key, `arcFromPBK `contains the arc from the per-boot key to the private key.
     *
     * @param keyType: the type of the payload key to be created (i.e. symmetric or asymmetric). If
     * it is symmetric, key should be an AES-256 key. If it is asymmetric, the should be an EC key
     * pair used for ECDH.
     *
     * @param permissions: the arcs from which the permission should be inherited to the newly
     * created arc. Such arc(s) should have the domain's per-boot key as the encrypting key.
     *
     * @return: Newly created `Key`.
     */
    KEResult create();

    /**
     * Given a peer’s ECDH public key, create one’s own ECDH key, compute a diffie-hellman shared
     * secret, derive the shared key (a symmetric encryption key) and compute an arc from the
     * per-boot key to the shared key, and return the created ECDH public key for the peer to derive
     * the same shared key.
     *
     * To support the security properties of an authenticated key exchange, include
     * the identity of the two parties in computing the the diffie-hellman shared secret, derive a
     * MAC key from the shared secret (in addition to the shared symmetric encryption key), compute
     * a session id by concatenating the two nonces and compute the signature over the session id
     * and return KEResult.
     */
    KEResult init(in PubKey peer_dh_key, in Identity peer_id, in byte[] nonce);

    /**
     * Given the arc containing one’s own ECDH private key created in a previous `create` call,
     * the peer’s ECDH public key returned in a previous `ke_init` call, compute a
     * diffie-hellman shared secret, derive the channel key and compute the channel arc (an arc from
     * the per-boot key to the channel key), and return the channel arc.
     *
     * Additionally, to support the security properties of an authenticated key exchange, compute
     * the signature on the concatenation of the nonce sent by the peer and one’s own ECDH
     * public key, derive a MAC key from the diffie-hellman shared secret in addition to the channel
     * key, compute the session id, compute MAC on its own identity.
     * Also, verify the `auth_key_binding` and the `signature` computed by the peer (which is
     * combined into the input:`auth_sign_mac`).
     */
    KEResult finish(in PubKey peer_dh_key, in Identity peer_id, in Signature peer_signature,
            in byte[] nonce, in Arc own_dh_key);

    /**
     * This is the last step of authenticated key exchange where the peer who executed `ke_init`
     * verifies the `auth_sign_mac` computed by the peer and returns the channel arc.
     */
    KEResult authComplete(in Signature peer_signature, in Arc[] session_keys);
}
