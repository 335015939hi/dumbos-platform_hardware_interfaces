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
 * The common interface of authgraph defines the methods that should be implemented by both source
 * domains and sink domains.
 *
 */
interface IAuthGraphCommon {
    /**
     * Initializes an ephemeral EC key pair required to establish a secret channel between
     * two domains and outputs the public key in InitChannelResult.
     * All domains participating in authgraph should maintain a key for every boot cycle, which is
     * called Per-Boot-Key (PBK).
     * Optionally, if a domain does not wish to maintain state across the invocations of the three
     * methods related to channel establishment, it can include an arc from PBK to the private key
     * of the EC key pair in the InitChannelResult.
     *
     * @return InitChannelResult
     */
    InitChannelResult initChannel();

    CreateChannelResult createChannel(
            in byte[] pubKeyOfOtherParty, in @nullable InitChannelResult initChannelResult);

    VerifyChannelResult verifyChannel(in byte[] signatureOfOtherParty,
            in @nullable VerifyChannelResult verifyChannelResult);
}
