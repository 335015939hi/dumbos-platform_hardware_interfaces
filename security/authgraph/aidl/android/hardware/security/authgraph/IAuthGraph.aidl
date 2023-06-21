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
import android.hardware.security.authgraph.KEResult;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.KeyType;
import android.hardware.security.authgraph.PubKey;

/**
 * Authgraph interface definition.
 *
 * Authgraph enables one domain (sink domain) to encrypt its resources with a secret belonging to
 * another domain (source domain), such that the sink domain's resource can not
 * be used without the source domain's secret being available. The source domain's secrets used to
 * encrypt/decrypt the sink domain's resources are communicated to the sink domain via a secure
 * channel established between the two domains, because such communication usually happens via the
 * non-secure world.
 * E.g. An auth-bound key created in KeyMint TA which requires user's password authentication in key
 * usage, is encrypted using a key known to gatekeeper TA. Such key is encrypted using a key derived
 * from the user's password, such that the auth-bound key created in Keymint is cryptographically
 * bound to the user's password.
 *
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraph {
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
    Key create(in KeyType keyType, in @nullable Arc[] permission);

    /**
     * Given a peer’s ECDH public key, create one’s own ECDH key, compute a diffie-hellman shared
     * secret, derive the shared key (a symmetric encryption key) and compute an arc from the
     * per-boot key to the shared key, and return the created ECDH public key for the peer to derive
     * the same shared key.
     *
     * Additionally, to support the security properties of an authenticated key exchange, create
     * a nonce, compute the signature on the concatenation of the nonce sent by the peer and its own
     * ECDH public key, derive a MAC key from the diffie-hellman shared secret
     * (in addition to the shared symmetric encryption key), compute the session id, compute MAC on
     * its own identity and return KEResult.
     */
    KEResult keInit(in PubKey peer_dh_key, in @nullable Arc[] permissions);

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
    KEResult keFinish(in PubKey peer_dh_key, in Arc own_dh_key,
            in @nullable Arc[] permissions,
            in @nullable AuthenticatedBinding auth_sign_mac);

    /**
     * This is the last step of authenticated key exchange where the peer who executed `ke_init`
     * verifies the `auth_sign_mac` computed by the peer and returns the channel arc.
     */
    KEResult keAuthComplete();

    /**
     * Given two arcs from the per-boot key, return an arc from the payload key of the first arc to
     * the payload key of the second arc. The output arc will be an input to the second argument of
     * the snap operation in subsequent method calls of Authgraph (see the definition of the snap
     * operation).
     *
     * @param encryptingKey: an arc from the domain’s per-boot key to the encrypting key of the
     *                       output arc.
     *
     * @param toBeEncryptedKey: an arc from the per boot key to the key to be sencrypted in the
     *                          output arc (i.e. payload key in the output arc)
     *
     * @return an arc from the encrypting key to the to be encrypted key.
     */
    Arc mint(in Arc encryptingKey, in Arc toBeEncryptedKey);

    /**
     * Given two arcs in which the first arc is from the per-boot key to the key that is the
     * encrypting key of the second arc, return an arc from the per-boot key to the key that is
     * being encrypted in the second arc.
     *
     * @param decryptingkey: an arc from the TA’s per-boot key to the encrypting key of the
     *                       second argument.
     * @param encryptedKey: an arc from the encrypting key (i.e. the key that is encrypted in the
     *                      arc of the first argument) to the encrypted key (i.e. the payload key in
     *                      the returned arc). This arc is a result of a previous mint operation.
     *
     * @return an arc from the per-boot key to the key that is encrypted in the second argument
     *
     */
    Arc snap(in Arc decryptingKey, in Arc encryptedKey);
}
