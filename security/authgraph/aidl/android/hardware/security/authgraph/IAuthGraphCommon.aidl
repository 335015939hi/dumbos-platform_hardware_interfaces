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
import android.hardware.security.authgraph.FinalizeChannelResult;
import android.hardware.security.authgraph.InitChannelResult;

/**
 * Authgraph Common interface definition.
 *
 * Authgraph enables one domain (sink domain) to encrypt it's secrets with a key derived from a
 * secret that belongs to another domain (source domain), such that the sink domain's secret can not
 * be decrypted without the source domain's secret being available. And when the source domain's
 * secret is available, it can be communicated to the sink domain via a secure channel
 * pre-established beteween the two domains. E.g. A key created in KeyMint TA is encrypted using a
 * key derived from the fingerprint secret that resides in fingerpirnt TA, s.t. a user's keys are
 * cryptographically bound to user authentication.
 *
 * The messages exchanged beteween the domains are called Arcs. An arc is simply AES-GCM encryption
 * of a message M with a key K and additional authentication data D (i.e. Arc = Enc(K, D, M)).
 *
 * The common interface of authgraph defines the methods that should be implemented by both source
 * domains and sink domains.
 * @hide
 */
interface IAuthGraphCommon {
    /**
     * Initializes an ephemeral elliptic curve (EC) key pair required to establish a secret channel
     * between two domains and outputs the public key in InitChannelResult.
     * All domains participating in authgraph should maintain a key for every boot cycle, which is
     * called Per-Boot-Key (PBK). Optionally, if a domain does not wish to maintain state across the
     * invocations of the three methods related to channel establishment, it may include an arc from
     * PBK to the private key of the EC key pair in the field: 'arcFromPerBootKeyToPrivateKey' of
     * InitChannelResult, which will be then included in the optional input parameter:
     * 'initChannelResult' of createChannel() method.
     *
     * ErrorCode::OPERATION_NOT_SUPPORTED must be returned as a ServiceSpecificException
     * if the method is not implemented.
     *
     * @return InitChannelResult
     */
    InitChannelResult initChannel();

    /**
     * Computes two keys via elliptic curve diffie hellman (ECDH) key agreement which are used as
     * channel keys agreed by the two domains. One key is to encrypt incoming messages and the other
     * key is to encrypt outgoing messages.
     *
     * Domains should output an arc from PBK to each channel key, attaching the transcript of the
     * ECDH key agreement and the relevant label ('incoming'/'outgoing') in D. These are called
     * channel arcs. Transcript is the concatenation of the EC public key of self and that of the
     * other party. Domains should also output the signature on the transcript, created using their
     * signing key. These outputs are included in the returned type CreateChannelResult.
     *
     * ErrorCode::INVALID_EC_KEY must be returned if an invalid input is provided as
     * pubKeyOfOtherParty. ErrorCode::OPERATION_NOT_SUPPORTED must be returned if the method is not
     * implemented. Error codes are returned as ServiceSpecificExceptions.
     *
     * @param pubKeyOfOtherParty: EC public key of the other party that is used to compute channel
     *        keys via ECDH.
     *
     * @param initChannelResult: InitChannelResult returned from initiChannel() method. This is
     *        optional, unless the field: 'arcFromPerBootKeyToPrivateKey' of InitChannelResult
     *        returned from initiChannel() method is not empty.
     *
     * @return CreateChannelResult
     */
    CreateChannelResult createChannel(
            in byte[] pubKeyOfOtherParty, in @nullable InitChannelResult initChannelResult);

    /**
     * Verifies the signature of the other party on the transcript and if successful, updates the
     * channel arcs by attaching the public signing key of the other party in D, instead of the
     * transcript. The updated and finalized channel arcs are returned in FinalizeChannelResult.
     *
     * ErrorCode::INVALID_SIGNATURE must be returned if the input signature of the other party is
     * invalid. ErrorCode::OPERATION_NOT_SUPPORTED must be returned if the method is not
     * implemented. Error codes are returned as ServiceSpecificExceptions.
     *
     * @param signatureOfOtherParty: Signature of the other party on the ECDH key agreement
     *        transcript.
     *
     * @param createChannelResult: CreateChannelResult returned in createChannel() method.
     *
     * @return FinalizeChannelResult
     */
    FinalizeChannelResult finalizeChannel(
            in byte[] signatureOfOtherParty, in CreateChannelResult createChannelResult);
}
