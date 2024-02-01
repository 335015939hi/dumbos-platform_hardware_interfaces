/*
 * Copyright (C) 2024 The Android Open Source Project
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
import android.hardware.security.authgraph.AuthKeyPackage;
import android.hardware.security.authgraph.Key;
import android.hardware.security.authgraph.KeyWrappingKeyHandle;
import android.hardware.security.authgraph.KeyWrappingKeyPackage;

/**
 * This interfaces defines a protocol for cryptographically protecting one party's (sink's) secrets
 * with a key held by the other party (source), where the sink and the source exchange protocol
 * messages via untrusted parties. Such cryptographically protected secrets at the sink can be
 * decrypted if any only if the key held by the source is unlocked. The mechanism of locking and
 * unlocking the key held at the source is specific to the operational logic of the source and is
 * outside of the scope of IAuthGraphKeyManagement protocol.
 *
 * Pre-requisites:
 *     1. Each pair of source and sink should have:
 *              i. a shared key setup by executing the key exchange protocol defined in
 *                 `IAuthGraphKeyExchange` API.
 *             ii. a long term encryption key owned by each party
 *            iii. a symmetric encryption key kept in memory with per-boot life time of the
 *                 participant (a.k.a per-boot key)
 *     2. A user who wants to create a secret at the sink, that is protected by the source, should
 *        have setup a unique key at the source (called "source key"). The source key is encrypted
 *        a with secret known to the user and/or with the source's long term encryption key.
 *
 * This protocol, combined with the protocol defined in `IAuthGraphKeyExchange` API, provides the
 * following security guarantees:
 *     1. Any compromised party via whom the source and sink communicate, cannot trick the sink into
 *        creating a secret protected by a non-legitimate key by pretending that it is issued by the
 *        genuine source (during creation of the protected secrets, it is assumed that both the
 *        source and sink are not compromised).
 *     2. Even if the sink is compromised later on, the sink cannot unlock the protected
 *        secrets created at the sink, as long as the sink does not have access to the source's key.
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
@VintfStability
interface IAuthGraphKeyManagement {
    /**
     * This method is invoked on a source, right after the user authenticates with the source and
     * the source key is unlocked.
     * This method creates a unique auth key for a particular sink. The sink should have setup a
     * shared key with the source via the protocol defined in IAuthGraphKeyExchange. If an auth key
     * has already been created, this method unlocks the auth key for the sink.
     *
     * Perform the following steps:
     *     1. CHECK whether `sourceKey` is an arc that is encrypted with the source's per-boot
     *        key and has KeyType = SourceKey in the protected headers.
     *     2. For each AuthKeyPackage in `authKeyPackages`:
     *            i. CHECK whether `sharedKey` is an arc that is encrypted with the source's
     *               per-boot key and has KeyType = SharedKey and "sink_id" in the protected
     *               headers.
     *           ii. CHECK that there is only one AuthKeyPackage for a given sink
     *          iii. If an `authKey` is present;
     *                   a. CHECK whether `persistent` field in the AutheKey is an arc that is
     *                      encrypted with the source key (i.e. the payload key of input 1) and
     *                      has KeyType = AuthKey_w_SourceKey in the protected headers.
     *                   b. CHECK whether the "sink_id" in the protected headers of `sharedKey` in
     *                      the AuthKeyPackage matches the identity verification policy in the
     *                      "minting_allowed" protected header of the `persistent` arc.
     *                   c. Create an arc that encrypts the auth key with the shared key and add
     *                      the following in the protected headers:
     *                      i.  source_id = `Identity` of the source
     *                      ii. KeyType = AuthKey_w_SharedKey
     *                   d. Create an AuthKeyPackage to be returned to the caller, with `sharedKey`
     *                      from the input AuthKeyPackage and an `authKey` with no `persistent` arc
     *                      and the arc created in step c above being the `ephemeral` arc.
     *                   e. Add the AuthKeyPackage created in step d above to the array of
     *                      AuthKeyPackages to be returned.
     *           iv. If an `authKey` is not present:
     *                   a. Create a unique auth key for the sink
     *                   b. Create an arc that encrypts the auth key with the shared key and add
     *                      the following in the protected headers:
     *                      i.  source_id = `Identity` of the source
     *                      ii. KeyType = AuthKey_w_SharedKey
     *                   c. Create an arc that encrypts the auth key with the source key and add
     *                      the following in the protected headers:
     *                      i.  minting_allowed = `Identity` of the sink
     *                      ii. KeyType = AuthKey_w_SourceKey
     *                   d. Create an AuthKeyPackage to be returned to the caller, with `sharedKey`
     *                      from the input AuthKeyPackage and an `authKey` with the arc created in
     *                      step b above being the `persistent` arc and the arc created in step c
     *                      above being the `ephemeral` arc.
     *                   e. Add the AuthKeyPackage created in step d above to the array of
     *                      AuthKeyPackages to be returned.
     *       3. Return the array of AuthKeyPackages created in step 3.
     *
     * Note: If any of the enforcements marked as `CHECK` above fails, return the error:
     * ENFORCEMENTS_FAILED
     *
     * @param sourceKey - an arc encrypting the user's source key with the source's per-boot key
     *
     * @param authKeyPackages - an array of AuthKeyPackages, each AuthKeyPackage corresponds to
     *                          a sink that has setup a shared key with the source via the
     *                          protocol defined in IAuthGraphKeyExchange.
     *
     * @return AuthKeyPackage - an array of AuthKeyPackages, each AuthKeyPackage corresponds to
     *                          a sink that has setup a shared key with the source via the
     *                          protocol defined in IAuthGraphKeyExchange and the source has
     *                          setup an auth key for each of those sinks at the return of this
     *                          function call.
     */
    AuthKeyPackage[] createOrUnlockAuthKeys(in Arc sourceKey, in AuthKeyPackage[] authKeyPackages);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to create a key pair whose public key is used in the process of
     * encrypting any secret key material created at the sink when the auth key provided by the
     * source is not available.
     *
     * Perform the following steps:
     *     1. CHECK whether `sharedKey` is an arc that is encrypted with the sink's per-boot key
     *        and has KeyType = SharedKey and "source_id" in the protected headers.
     *     2. CHECK whether `authKey` is an arc that is encrypted with the key shared between the
     *        sink and the source (i.e. the payload key in `sharedKey`) and has
     *        KeyType = AuthKey_w_SharedKey and "source_id" in the protected headers.
     *     3. CHECK whether the two "source_id"s in the protected headers of the two input arcs are
     *        equal.
     *     4. Create an EC key pair on NIST curve P-256 for ECDH.
     *     5. Encode the public key of the key pair from step 4 above as a COSE key and add the
     *        following custom headers:
     *            i. KeyType = PrimaryExchangePubKey
     *           ii. source_id = "source_id" from the input arcs
     *     6. Create an arc with PayloadType = AllZeroes and the encrypting key being the long term
     *        encryption key of the sink. Include the serialized COSE key from step 5 above in
     *        the protected headers of the arc as: "PlainPubKeyEncoded".
     *     7. Create an arc that encrypts the private key of the key pair from step 4 above, with
     *        the auth key (i.e. payload key in `authKey`) and add the following in the protected
     *        headers:
     *            i. KeyType = SecondaryExchangePubKey
     *           ii. source_id = "source_id" from the input arcs
     *     8. Create a `Key` to be returned with the authenticated public key from step 6 above
     *        and the encrypted private key from step 7 above.
     *
     * Note: If any of the enforcements marked as `CHECK` above fails, return the error:
     * ENFORCEMENTS_FAILED
     *
     * @param sharedKey - an arc encrypting the key shared between the sink and the source, with the
     *                    source's per-boot key
     *
     * @param authKey - an arc encrypting the auth key created by the source for the sink, with the
     *                  key shared between the sink and the source (i.e. the payload key in
     *                  `sharedKey`)
     *
     * @return Key - a key pair created by the sink to be used for ECDH based key derivation of the
     *               "key wrapping key" when the auth key is not unlocked. The public key of the key
     *               pair is authenticated with the sink's long term encryption key and the private
     *               key of the key pair is encrypted with the auth key (i.e. the payload key in
     *               `authKey`)
     */
    Key setupAuthKeyPair(in Arc sharedKey, in Arc authKey);

    KeyWrappingKeyPackage createKeyWrappingKey(
            in Arc keyWrappingKeyInput, in @nullable Arc sharedKey);

    KeyWrappingKeyPackage recoverKeyWrappingKey(
            in KeyWrappingKeyHandle keyWrappingKeyHandle, in Arc authKey);
}
