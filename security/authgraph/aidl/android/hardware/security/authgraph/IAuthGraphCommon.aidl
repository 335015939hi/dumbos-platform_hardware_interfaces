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

import android.hardware.security.authgraph.CreateChannelResult;
import android.hardware.security.authgraph.InitChannelResult;

/**
 * Authgraph Common interface definition.
 *
 * Authgraph enables one domain (sink domain) to encrypt it's secrets with a key derived from a
 * secret that belongs to another domain (source domain), such that the sink domain's secret can not
 * be decrypted without the source domain's secret being available. And when the source domain's
 * secret is available, it can be communicated to the sink domain via a secure channel
 * pre-established between the two domains. E.g. A key created in KeyMint TA is encrypted using a
 * key derived from the fingerprint secret that resides in fingerprint TA, s.t. a user's keys are
 * cryptographically bound to user authentication.
 *
 * The messages exchanged between the domains are called Arcs. An arc is simply AES-GCM encryption
 * of a message M with a key K and additional authentication data D (i.e. Arc = Enc(K, D, M)).
 *
 * The common interface of authgraph defines the methods that should be implemented by both source
 * domains and sink domains.
 *
 * Channel establishment between two domains is orchestrated by some system component such as
 * keystore.
 *
 * ErrorCodes are defined in android.hardware.security.keymint.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphCommon {
    /**
     * Initializes an ephemeral elliptic curve (EC) key pair, with P-256, in order to establish
     * a secret channel between two domains via elliptic curve diffie hellman (ECDH) key agreement.
     * The signed public key of the key pair is included in InitChannelResult. The signature is
     * created using the persistent signing key of the domain, via ECDSA.
     * All domains participating in authgraph should maintain a symmetric key for every boot cycle,
     * which is called Per-Boot-Key (PBK). Optionally, if a domain does not wish to maintain state
     * across the invocations of the two methods related to channel establishment, it may include an
     * arc from PBK to the private key of the EC key pair in the field:
     * 'arcFromPerBootKeyToPrivateKey' of InitChannelResult, which will be then included in the
     * optional input parameter: 'initChannelResult' of createChannel() method.
     *
     * ErrorCode::OPERATION_NOT_SUPPORTED must be returned as a ServiceSpecificException
     * if the method is not implemented.
     *
     * @param signingKeySetupInfo: Each domain is supposed to have a persistent signing key (EC
     *         P-256),
     *        which is handed over to them via BCC (Boot Certificate Chain).
     *        Since BCC is not available yet, this optional input parameter provides means by which
     *        a domain can obtain a signing key.
     *
     * @return InitChannelResult
     */
    InitChannelResult initChannel(in @nullable byte[] signingKeySetupInfo);

    /**
     * Verifies the signature on the ephemeral EC public key of the other party, which is supplied
     * as the first argument. If the verification is successful, derives a secret via ECDH and
     * computes three keys via KDF. The first two are used as the channel keys agreed by the two
     * domains. One key is to used to decrypt incoming messages and the other key is to encrypt
     * outgoing message. The third derived key is used to compute MAC on a fixed value, in order to
     * allow the caller to verify that ECDH key agreement and derivation was successful at both
     * parties by comparing the MAC values provided by the two domains.
     *
     * Domains should output an arc from PBK to each channel key, attaching the public signing key
     * of the other party in D. These are called channel arcs. These outputs are included in the
     * returned type CreateChannelResult.
     *
     * ErrorCode::INVALID_SIGNATURE must be returned if the input signature of the other party is
     * invalid. INVALID_EC_KEY must be returned if an invalid key is provided in
     * signedPublicKeyOfOtherParty. ErrorCode::OPERATION_NOT_SUPPORTED must be returned if the
     * method is not implemented. Error codes are returned as ServiceSpecificExceptions.
     *
     * @param signedPublicKeyOfOtherParty: Signed EC public key of the other party that is used to
     *        compute the channel keys via ECDH.
     *
     * @param initChannelResult: InitChannelResult returned from initChannel() method. This is
     *        optional, unless the field: 'arcFromPerBootKeyToPrivateKey' of InitChannelResult
     *        returned from initiChannel() method is not empty.
     *
     * @return CreateChannelResult
     */
    CreateChannelResult createChannel(in byte[] signedPublicKeyOfOtherParty,
            in @nullable InitChannelResult initChannelResult);
}
