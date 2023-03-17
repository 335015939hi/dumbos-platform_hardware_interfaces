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

import android.hardware.security.authgraph.CreateChannelResult;
import android.hardware.security.authgraph.InitChannelResult;

/**
 * Authgraph Common interface definition.
 *
 * Authgraph enables one domain (sink domain) to encrypt its resources with a secret belonging to
 * another domain (source domain), such that the sink domain's resource can not
 * be used without the source domain's secret being available. The source domain's secrets used to
 * wrap/unwrap the sink domain's resources are communicated to the sink domain via a secure channel
 * established between the two domains, because such communication usually happens via the
 * non-secure world.
 * E.g. An auth-bound key created in KeyMint TA is encrypted using a key derived from the
 * fingerprint secret that resides in Fingerprint TA, such that the user's key is
 * cryptographically bound to user authentication.
 *
 * Most of the messages exchanged between the domains are called Arcs. An arc is simply AES-GCM
 * encryption of a payload P with a key K and additional authentication data (AAD) D
 * (i.e. Arc = Enc(K, P, D)).
 * The CDDL of an arc is as follows.
 * Arc = [
 *     meta_info : bstr .cbor MetaInfo,  ; permissions and limitations as external AAD.
 *     encrypted_node : EncryptedNode,   ; encrypted key/Arc
 * ]
 *
 * MetaInfo = [
 *     arc_type: ArcType			       ; type of the arc
 *     permissions: [Permission],
 *     limitations: [Limitation],
 * ]
 *
 * ArcType = &(
 *     channel_key_arc: 1,
 *     private_key_arc: 2,
 *     km_key_arc: 3,
 *     pub_wrapping_key_arc: 4,
 *     locked_pub_wrapping_key_arc: 5,
 *     locked_sym_wrapping_key_arc: 6,
 *     unlocked_pub_wrapping_key_arc: 7,
 *     unlocked_sym_wrapping_key_arc: 8,
 * )
 *
 *
 *
 *
 * This interface of authgraph defines the methods that should be implemented by both source
 * domains and sink domains in order to be able to establish a secure channel.
 *
 * Channel establishment between two domains is usually orchestrated by some system component such
 * as Keystore. Keystore should be aware/informed of the pairs of domains that need to establish
 * a secure channel with each other.
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphCommon {
    /**
     * Creates an ephemeral elliptic curve (EC) key pair, with P-256, in order to establish
     * a secret channel between two domains via elliptic curve diffie hellman (ECDH) key agreement.
     * The signed public key of the key pair is returned in InitChannelResult. The signature is
     * created using the signing key of the domain, via ECDSA.
     * All domains participating in authgraph should have a symmetric key for every boot cycle,
     * which is called Per-Boot-Key (PBK). Optionally, if a domain does not wish to maintain state
     * across the invocations of the two methods related to channel establishment, it may include an
     * arc from PBK to the private key of the EC key pair in the field:
     * 'arcFromPerBootKeyToPrivateKey' of InitChannelResult, which will be then included in the
     * optional input parameter: 'initChannelResult' of createChannel() method.
     *
     * ErrorCode::OPERATION_NOT_SUPPORTED must be returned as a ServiceSpecificException
     * if the method is not implemented.
     *
     * @param signingKeySetupInfo: Each domain is supposed to have a signing key (EC
     *        P-256), which is handed over to them as a DICE artifact.
     *        If a domain does not have DICE artifacts, this optional input parameter provides means
     *        by which a domain can obtain a signing key.
     *
     * @return InitChannelResult
     */
    InitChannelResult initChannel(in @nullable byte[] signingKeySetupInfo);

    /**
     * Verifies the signature on the ephemeral EC public key of the other party, which is supplied
     * as the first argument. If the verification is successful, derives a secret via ECDH and
     * computes two keys via KDF.
     * The first key is used as the channel key agreed by the two domains. It is a 256-bit AES key
     * to be used in AES-GCM mode.
     * The second key is a 256-bit HMAC key to be used to compute HMAC on a fixed value, in order to
     * allow the caller to verify that ECDH key agreement was successful at both parties by
     * comparing the HMAC values provided by the two domains.
     *
     * Domains should output an arc from PBK to the channel key, attaching the public signing key
     * of the other party as AAD. This is called a channel arc, which is returned in
     * CreateChannelResult.
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
