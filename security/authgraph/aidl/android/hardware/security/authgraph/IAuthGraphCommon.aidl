/*
 * Copyright (C) 2020 The Android Open Source Project
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
import android.hardware.security.authgraph.VerifyChannelResult;

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
 * of a message M with a key K and additional authentication data D.
 *
 * The common interface of authgraph defines the methods that should be implemented by both source
 * domains and sink domains.
 *
 */
interface IAuthGraphCommon {
    /**
     * Initializes an ephemeral elliptic curve (EC) key pair required to establish a secret channel
     * between two domains and outputs the public key in InitChannelResult.
     * All domains participating in authgraph should maintain a key for every boot cycle, which is
     * called Per-Boot-Key (PBK).
     * Optionally, if a domain does not wish to maintain state across the invocations of the three
     * methods related to channel establishment, it may include an arc from PBK to the private key
     * of the EC key pair in the InitChannelResult, which will be included in the optional input
     * parameter of createChannel() method.
     *
     * @return InitChannelResult
     */
    InitChannelResult initChannel();

    /**
     * Computes two keys via elliptic curve diffie hellman (ECDH) which are used as channel keys
     * shared between the two domains.
     * One key is to encrypt incoming messages and the other key is to encrypt outgoing messages.
     * Domains should output two arcs from PBK to each channel key, attaching the transcript in D.
     * Transcript = EC public key of self || EC public key of the other party.
     * Domains should also output the signature on the transcript, created using their signing key.
     * These output are included in the returned type CreateChannelResult.
     *
     * @param EC public key of the other party that is used to compute channel keys via ECDH.
     *
     * @param Optional InitChannelResult if it was returned in initiChannel() method.
     *
     * @return CreateChannelResult
     */
    CreateChannelResult createChannel(
            in byte[] pubKeyOfOtherParty, in @nullable InitChannelResult initChannelResult);

    VerifyChannelResult verifyChannel(in byte[] signatureOfOtherParty,
            in @nullable VerifyChannelResult verifyChannelResult);
}
