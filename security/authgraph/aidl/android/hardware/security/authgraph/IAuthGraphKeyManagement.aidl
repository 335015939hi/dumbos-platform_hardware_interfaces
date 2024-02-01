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
import android.hardware.security.authgraph.AuthKeyPairPackage;
import android.hardware.security.authgraph.KeyWrappingKeyHandle;
import android.hardware.security.authgraph.KeyWrappingKeyPackage;

@VintfStability
interface IAuthGraphKeyManagement {
    /**
     * This method is invoked on a source, right after a user authenticates with the source.
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
     *     1.
     */
    AuthKeyPairPackage setupAuthKeyPair(in Arc sharedKey, in Arc authKey);

    KeyWrappingKeyPackage createKeyWrappingKey(
            in Arc keyWrappingKeyInput, in @nullable Arc sharedKey);

    KeyWrappingKeyPackage recoverKeyWrappingKey(
            in KeyWrappingKeyHandle keyWrappingKeyHandle, in Arc authKey);

    /**
     * This method is invoked on a sink TA.
     * This is a helper method for the sink TA to decrypt the auth key (sent by the source TA) from
     * and re-encrypt with the sink TA's per-boot key.
     * @param sharedKey - arc encrypting (with the sink'd per-boot key) the shared key setup with
     *                     source TA who creates the `authKey` arc
     * @param authKey - arc encrypting (with the shared key setup with the source TA) the auth
     *                  key created by the source TA
     *
     * @return Arc - arc encrypting the auth key with the sink TA's per-boot key
     */
    Arc unwrapAuthKey(in Arc sharedKey, in Arc authKey);
}
