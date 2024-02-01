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
import android.hardware.security.authgraph.KeyWrappingKeyDeriveInfo;
import android.hardware.security.authgraph.KeyWrappingKeyPackage;

/**
 * This interface defines a protocol for a sink to obtain a symmetric encryption key that is
 * cryptographically protected with a key held by a source, where the sink and the source exchange
 * protocol messages asynchronously via untrusted parties.
 *
 * The sink may use the key obtained from this protocol to encrypt a secret material that a user may
 * create at the sink by calling some sink specific functionality, which is outside the scope of the
 * IAuthGraphKeyManagement protocol.
 *
 * Such cryptographically protected user secrets created at the sink can be decrypted if any only if
 * the key held by the source is unlocked by the user.
 *
 * The mechanism of locking and unlocking the key held at the source is specific to the operational
 * logic of the source and therefore, is outside of the scope of the IAuthGraphKeyManagement
 * protocol.
 *
 * Pre-requisites:
 *     1. Each pair of source and sink should have:
 *              i. a shared key setup by executing the key exchange protocol defined in
 *                 `IAuthGraphKeyExchange` API.
 *             ii. a long term encryption key owned by each party
 *            iii. a symmetric encryption key kept in memory with per-boot life time of the
 *                 participant (a.k.a per-boot key)
 *     2. A user who wants to create a secret at the sink, that is protected by a key held by the
 *        source, should have setup a unique key at the source (called "source key"), via a source
 *        specific functionality.
 *
 * This protocol, combined with the protocol defined in `IAuthGraphKeyExchange` API, provides the
 * following security guarantees:
 *     1. Any compromised party via whom the source and sink communicate, cannot trick the sink into
 *        creating a user secret that is encrypted with a non-legitimate key, by pretending that it
 *        is issued by the genuine source (during creation of the protected secrets, it is assumed
 *        that both the source and sink are not compromised).
 *     2. Even if the sink is compromised later on, the sink cannot unlock the protected user
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
     * has already been created, this method unlocks the auth key for the sink. The auth key is
     * encrypted by the aforementioned source key, producing a persistent arc. There can be multiple
     * auth keys created by the source for multiple sinks, that are encrypted by the same
     * source key.
     *
     * Perform the following steps:
     *     1. CHECK whether the input: `sourceKey` is an arc that is encrypted with the source's
     *        per-boot key and has KeyType = SourceKey in the protected headers.
     *     2. For each AuthKeyPackage in the input: `authKeyPackages`:
     *            i. CHECK whether `sharedKey` is an arc that is encrypted with the source's
     *               per-boot key and has KeyType = SharedKey and "sink_id" in the protected
     *               headers.
     *           ii. CHECK that there is only one AuthKeyPackage for a given sink
     *          iii. If an `authKey` is present;
     *                   a. CHECK whether `persistent` field in the AutheKey is an arc that is
     *                      encrypted with the source key (i.e. the payload key of input 1) and
     *                      has KeyType = AuthKey_w_SourceKey in the protected headers.
     *                   b. CHECK whether the "sink_id" in the protected headers of the `sharedKey`
     *                      field in the AuthKeyPackage matches the identity verification policy in
     *                      the "minting_allowed" protected header of the `persistent` arc of the
     *                      AuthKey.
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
     *                   a. Create a 256-bit cryptographic key for AES-GCM encryption (auth key)
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
     *       3. Return the array of AuthKeyPackages created in step 2.
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
     *                          setup an auth key for, at the return of this function call.
     */
    AuthKeyPackage[] createOrUnlockAuthKeys(in Arc sourceKey, in AuthKeyPackage[] authKeyPackages);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to create a key pair whose public key is used to derive a
     * symmetric encryption key encrypt any secret key material of the user created at the sink when
     * the auth key provided by the source is locked.
     * This method is not required to be implemented if there is no need to create a user's secret
     * key material at the sink when the corresponding user's secret at the source is locked.
     *
     * Perform the following steps:
     *     1. CHECK whether the input: `sharedKey` is an arc that is encrypted with the sink's
     *        per-boot key and has KeyType = SharedKey and "source_id" in the protected headers.
     *     2. CHECK whether the input: `authKey` is an arc that is encrypted with the key shared
     *        between the sink and the source (i.e. the payload key of input 1) and has
     *        KeyType = AuthKey_w_SharedKey and "source_id" in the protected headers.
     *     3. CHECK whether the two "source_id"s in the protected headers of the two input arcs are
     *        equal.
     *     4. Create an EC key pair on NIST curve: P-256 for ECDH.
     *     5. Encode the public key of the key pair from step 4 above as a COSE key and add the
     *        following custom header: KeyType = PrimaryExchangePubKey
     *     6. Create an arc with PayloadType = Empty and the encrypting key being the long term
     *        encryption key of the sink. Add the following protected headers:
     *            i. PlainPubKeyEncoded = the serialized COSE key from step 5 above
     *           ii. source_id = "source_id" from the input arcs
     *          iii. sink_id = `Identity` of the sink
     *     7. Create an arc that encrypts the private key of the key pair from step 4 above, with
     *        the auth key (i.e. payload key of input 2) and add the following in the protected
     *        headers:
     *            i. KeyType = PrimaryExchangePrivKey_w_AuthKey
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
     *               "key wrapping key" when the auth key is locked. The public key of the key
     *               pair is authenticated with the sink's long term encryption key and the private
     *               key of the key pair is encrypted with the auth key
     */
    Key setupAuthKeyPair(in Arc sharedKey, in Arc authKey);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to create a unique symmetric key that will be used to encrypt a
     * user's secret created at the sink when the auth key created at the source is locked.
     * In this case, the sink derives a symmetric key via ECDH using the public key of the key pair
     * created in `setupAuthKeyPair` whose private key is protected with the auth key.
     * This method is not required to be implemented if no secret material is created at the sink
     * when the corresponding user's secret at the source is locked.
     *
     * Perform the following steps:
     *    1. CHECK whether the input: `primaryExchangePubKey` is an arc that is encrypted with the
     *       sink's long term encryption key and has the protected header: `PlainPubKeyEncoded`
     *       (with a serialized COSE key as the value)
     *    2. CHECK in the headers of the COSE key has KeyType = PrimaryExchangePubKey,
     *       "source_id" and "sink_id"
     *    3. Deserialize the public key found in the protected header: `PlainPubKeyEncoded` of
     *       the input: `primaryExchangePubKey` arc
     *    4. Create an EC key pair on NIST curve: P-256 for ECDH
     *    5. Create a nonce
     *    6. Compute the Diffie-Hellman shared secret: Z, using the public key from step #3
     *       and the key pair created in step #4 above
     *    7. Extract a cryptographic secret S from Z, using the nonce created in step #5 as the
     *       salt
     *    8. Derive a symmetric encryption key of 256 bits from S with b"KEY_WRAPPING_KEY" as the
     *       context
     *    9. Derive a MAC key of 256 bits from S with b"KEY_WRAPPING_KEY_CONFIRMATION_KEY" as the
     *       context
     *   10. Compute "session_id" as the 256 bits HMAC over the nonce created in step #5 with
     *       the key derived in step #9
     *   11. Encode the public key of the key pair created in step #4 and add the custom
     *       header: KeyType = SecondaryExchangePubKey
     *   12. Create an arc with PayloadType = Empty and the encrypting key being the long term
     *       encryption key of the sink. Add the following protected headers:
     *         i. PlainPubKeyEncoded = the serialized COSE key from step #11 above
     *        ii. sink_id = `Identity` of the sink
     *       iii. Nonce = nonce created in step #5
     *        iv. session_id = the one computed in step #10
     *   13. Create an arc that encrypts the symmetric encryption key derived in step #8 with
     *       the per-boot key of the sink. Add the following protected headers:
     *        i. source_id = "source_id" from the input arc
     *       ii. KeyType = KeyWrappingKeyGen
     *   14. Create a `KeyWrappingKeyPackage` to be returned, containing the arc from step #13 as
     *       the `keyWrappingKey` and the arc from step #12 as the `keyWrappingKeyHandle`
     *
     * Note: If any of the enforcements marked as `CHECK` above fails, return the error:
     * ENFORCEMENTS_FAILED
     *
     * @param primaryExchangePubKey - an arc authenticating the public key of the key pair created
     *                                in `setupAuthKeyPair` with the sink's long term encryption key
     *
     * @return KeyWrappingKeyPackage - contains ephemeral arc containing the key wrapping key - to
     *                                 be used for encrypting the user's secret key material created
     *                                 at the sink and a persistent arc containing the authenticated
     *                                 public key created for ECDH based derivation of the key
     *                                 wrapping key - to be used for re-deriving the same key when
     *                                 unlocking the user's secret key material.
     */
    KeyWrappingKeyPackage createKeyWrappingKeyWithKeyExchange(in Arc primaryExchangePubKey);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to create a unique symmetric key that will be used to encrypt a
     * user's secret created at the sink when the auth key created at the source is unlocked.
     * In this case, the sink creates a new symmetric key and encrypt it with the auth key.
     *
     * Perform the following steps:
     *     1. CHECK whether `sharedKey` is an arc that is encrypted with the sink's per-boot key and
     *        has KeyType = SharedKey and "source_id" in the protected headers.
     *     2. CHECK whether `authKey` is an arc that is encrypted with the key shared between the
     *        sink and the source (i.e. the payload key of input 1) and has
     *        KeyType = AuthKey_w_SharedKey and "source_id" in the protected headers.
     *     3. CHECK whether the two "source_id"s in the protected headers of the two input arcs are
     *        equal.
     *     4. Create a 256-bit cryptographic key for AES-GCM encryption (key wrapping key).
     *     5. Create an arc that encrypts the auth key with the sink's per-boot key and add the
     *        following in the protected headers:
     *            i. source_id = "source_id" from the input arcs
     *           ii. KeyType = KeyWrappingKeyGen
     *     6. Create an arc that encrypts the auth key with the shared key and add the following in
     *        the protected headers:
     *            i. source_id = "source_id" from the input arcs
     *           ii. KeyType = KeyWrappingKeyGen_w_SharedKey
     *     7. Create a `KeyWrappingKeyPackage` to be returned, containing the arc from step #5 as
     *        the `keyWrappingKey` and the arc from step #6 as the `keyWrappingKeyHandle`
     *
     * Note: If any of the enforcements marked as `CHECK` above fails, return the error:
     * ENFORCEMENTS_FAILED
     *
     * @return KeyWrappingKeyPackage - contains ephemeral arc containing the key wrapping key - to
     *                                 be used for encrypting the user's secret key material created
     *                                 at the sink and a persistent arc containing the key wrapping
     *                                 key - to be used for re-deriving the same key when unlocking
     *                                 the user's secret key material.
     */
    KeyWrappingKeyPackage createKeyWrappingKey(in Arc sharedKey, in Arc authKey);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to re-derive the key wrapping key that was derived in
     * `createKeyWrappingKeyWithKeyExchange`, when the user's secret material encrypted with
     * that key needs to be unlocked, once the user's secret at the source is unlocked.
     */
    KeyWrappingKeyPackage recoverKeyWrappingKeyWithKeyExchange(
            in KeyWrappingKeyDeriveInfo keyWrappingKeyInfo, in Arc sharedKey, in Arc authKey);

    /**
     * This method is invoked on a sink.
     * The goal of this method is to decrypt the key wrapping key that was created in
     * `createKeyWrappingKey`, when the user's secret material encrypted with
     * that key needs to be unlocked, once the user's secret at the source is unlocked.
     */
    KeyWrappingKeyPackage recoverKeyWrappingKey(
            in Arc keyWrappingKeyHandle, in Arc sharedKey, in Arc authKey);
}
