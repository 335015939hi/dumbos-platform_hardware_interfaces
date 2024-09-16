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
import android.hardware.security.authgraph.Capability;
import android.hardware.security.authgraph.Role;

/*
 * The IAuthGraphKeyManagement interface defines a protocol to create, modify and unlock
 * an authentication graph (i.e. authgraph)  built between a source and a sink. The
 * authgraph is made up of connected arcs. An arc is simply a symmetric encryption (AES-GCM) of a
 * payload P with a key K and additional authentication data (AAD) D i.e. Arc = Enc(K, P, D).
 * Please refer Arc.aidl and Arc.cddl for more details.
 *
 * One of the main use-cases for this protocol is that the authgraph participant with a role
 * sink can obtain a symmetric encryption key that is cryptographically protected with a key
 * held by another participant with a role source, where the sink and the source exchange
 * protocol messages asynchronously via untrusted parties. The sink may use the key
 * obtained from this protocol to encrypt a secret material that a user may
 * create at the sink by calling some sink-specific functionality which is outside the scope of the
 * IAuthGraphKeyManagement protocol.
 *
 * This interface will be implemented by an authgraph participant which can be a source such as
 * GateWeaver, a sink such as KeyMint or both such as FRP. We call such a participant, which is an
 * instance of the IAuthGraphKeyManagement interface, a trusted application (TA) in this document.
 *
 * Such cryptographically-protected secrets created at the sink can be decrypted if and only if
 * the key held by the source is unlocked.
 *
 * The mechanism of locking and unlocking the key held at the source is specific to the operational
 * logic of the source and is outside of the scope of the IAuthGraphKeyManagement
 * protocol.
 *
 * Pre-requisites:
 *     1. Each TA should have following data which is used to
 *        create new arcs and validate parameters in the interface methods:
 *          - allowed_sources: SourceIds that are allowed by this instance.
 *          - allowed_sinks: SinkIds that are allowed bu this instance.
 *          - allowed_roles: Roles supported by this instance which are source, sink or both.
 *          - UID policy: Identifies whether this instance allows arcs with single or multiple UIDs.
 *          Note: SourceIds and SinkIds are instances of the Identity CBOR data structure as defined
 *                in the Identity.cddl.
 *     2. Each pair of source and sink should have:
 *              i. a shared key arc set up by executing the authenticated key exchange protocol
 *                 defined in `IAuthGraphKeyExchange` API.
 *             ii. a long-term encryption key owned by each party.
 *            iii. a symmetric encryption key kept in memory with per-boot life time of the
 *                 participant (a.k.a per-boot key).
 *
 *     3. A user who wants to create a secret at the sink, that is protected by
 *        a key held by the source, should have set up a key that is unlocked
 *        at the source (called "cred key"), via a source-specific functionality. The cred key
 *        is associated with the authenticated party via a unique identifier (called UID). The
 *        authenticated party can be user, server, etc.
 *
 * This protocol, combined with the protocol defined in the `IAuthGraphKeyExchange` API, provides
 * the following security guarantees:
 *     1. An untrusted party space cannot trick a sink into protecting a
 *        user resources by a illegitimate auth key (i.e. a key that is not
 *        directly bound to user authentication)”.
 *     2. Even if the sink is compromised after the key creation, the sink cannot unlock the
 *        protected user secrets created at the sink, as long as the sink does not have access to
 *        the source's key.
 *     3. Any component other than the source that created the original key that was used to protect
 *        the user's resource created at the sink, cannot help sink to unlock that key.
 *
 * ErrorCodes are defined in the android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
/* @hide */
@VintfStability
interface IAuthGraphKeyManagement {
    /*
     * This method creates a new arc with capabilities derived from wrappingArc
     * and childArcCaps. The new arc's payload is a new secret key which is
     * wrapped by the key in the payload (payload key) from the wrappingArc.
     *
     * The childArcCaps is a set of desired permissions and limitations (capability set).
     * These permissions and limitations constrains the usage of the arc e.g.
     * how the arc can be used and by which actors. Permission is a positive
     * statement about the encryption/decryption key K of the arc and its
     * characteristics about what that key can be used for. The limitation is a
     * negative statement about the arc's payload key K such as expiry time
     * after which key must not be used, maximum usage count of the key, etc.
     *
     * This operation can be invoked at the TA which is either source or sink.
     * The input wrappingArc to this operation must be encrypted by the per-boot
     * key of the TA.
     *
     * Before creating the new arc, the TA performs a series of checks and if
     * any check fails, the TA must return an ENFORCEMENTS_FAILED service specific error.
     *
     * 1. CHECK that wrappingArc is successfully decrypted using the per-boot key of TA.
     *    Successful decryption means that wrappingArc is valid and owned by the TA.
     *    CHECK that decrypted payload is AES GCM symmetric key.
     * 2. CHECK that the role requested in input parameter is supported by the TA's
     *    allowedRoles policy.
     * 3. If childArcCaps are defined then
     *    - CHECK that the requested sourceIds, sinkIds and UIDPolicy are
     *      supported by the TA's allowedSinks, allowedSources and uid policy.
     *
     * Next, create a new arc by merging the permissions and limitations of
     * wrappingArc and childArcCaps as follows:
     * 1. If the requested role is source then copy sourceId from wrappingArc's
     *    permissions to the new arc.
     * 2. If the requested role is sink then copy sinkId, if defined, from wrappingArc's
     *    permissions to the new arc. In the case where no sinkId is defined then
     *    CHECK the MintingAllowed policy is present in wrappingArc and sinkId
     *    dice policy is specified. Add the identity associated with the sinkId
     *    dice policy to the new arc's SinkId.
     * 2. Copy the MintingAllowed policy, if defined, from the wrappingArc's
     *    permission to the new arc except UIDPolicy.
     * 3. Append MintingAllowed policy from childArc to the new arc. While doing so
     *    CHECK that if MintingAllowed permission in both the wrappingArc and the
     *    childArcCaps contains SinkId policies then they are equal. Note, default
     *    UIDPolicy=Single if childArcCaps does not define it.
     * 4. If neither childArcCaps not wrappingArc contains MintingAllowed policy
     *    then use default MintingAllowed policy for new arc, which contains sourceId
     *    and sinkId policies corresponding to the respective identities in the
     *    permissions and UIDPolicy = Single.
     * 5. Add limitations, if defined, from the childArcCaps to the new arc.
     * 6. Generate AES GCM Symmetric Secret Key as the payload
     *
     * Finally, create the new arc by encrypting the payload with the wrappingArc's
     * payload key by using AES GCM encryption.
     *
     * @param wrappingArc - this is the encrypting arc with long term key payload.
     * @param childArcCaps - these are set of sourceIds, sinkIds and UIDPolicy.
     * @param role - this is role of the arc i.e. either source or sink.
     * @return the newly created arc.
     */
    android.hardware.security.authgraph.Arc create(
            in android.hardware.security.authgraph.Arc wrappingArc,
            in @nullable android.hardware.security.authgraph.Capability childArcCaps,
            in android.hardware.security.authgraph.Role role);

    /*
     * This method merges the parentArc with the childArc to generate a
     * new arc by using the parentArc's encrypting key to encrypt the childArc's
     * payload. This operation can be invoked on both source and sink TAs.
     * The parentArc's encrypting key must be a per-boot key and the childArc
     * must be encrypted by the parentArc's payload key.
     *
     * Before creating the new Arc, the TA performs a series of checks to
     * verify that the creation should be allowed. If any check fails, the TA must
     * return an ENFORCEMENTS_FAILED service specific error is returned.
     *
     * Perform following steps with respect to parentArc and childArc:
     * 1. CHECK that parentArc is successfully decrypted using the per-boot key of
     *    TA and AES GCM decryption. Successful decryption means that parentArc
     *    is valid and owned by the TA. Extract the key from payload (payload key)
     *    and CHECK that childArc is successfully decrypted using this payload key
     *    and AES GCM decryption. Successful decryption means that childArc
     *    is valid and owned by the TA.
     * 2. CHECK that childArc has MintingAllowed policy defined and parentArc has
     *    at least sourceId or sinkId defined.
     * 3. CHECK that childArc's mintingAllowed sourceIds and sinkId policies
     *    are supported by sourceId and sinkId identity's dice certificate chain
     *    in the parentArc's permission.
     * 4. CHECK that those supported policies from childArc matches the sourceId and sinkId
     *    identity's dice policy from parentArc. If they do not match then then return
     *    service specific response KEY_ROTATION_REQUIRED.
     *
     * Next, create new arc as follows:
     * 1. Add the sourceId and sinkId permissions from parentArc to new arc's
     *     permissions.
     * 2. Add the MintingAllowed policy from childArc to newArc's permissions.
     * 3. Add UIDs from childArc to new arc.
     * 4. Add the limitations to newArc if either parentArc or childArc defines it.
     *    If both parentArc and childArc defines the limitations then merge the
     *    limitations add that to new arc, such that the new limitations are
     *    greater than or equal to parentArc's limitations as follows:
     *       - If challenge is defined in both arcs then include the challenge
     *         from childArc else include the one defined.
     *       - If timeout constraint is defined in both the arcs then include the lesser
     *         value of timeout.
     *       - If maxUsageCount constraint is defined in both the arcs then include the
     *         lesser value of maxUsageCount.
     * 5. Copy pauload from childArc to new arc.
     *
     * Finally create the new arc by using per-boot key of the TA to encrypt the payload
     * of the child Arc using AES GCM encryption.
     *
     * @param parentArc - this is the arc which is encrypted by the per-boot key.
     * @param childArc - this is the arc encrypted by the key in parent arc's payload.
     * @return the newly created arc encrypted by per-boot key containing payload
                from the childArc.
     */
    android.hardware.security.authgraph.Arc snap(
            in android.hardware.security.authgraph.Arc parentArc,
            in android.hardware.security.authgraph.Arc childArc);

    /*
     * This method creates a new arc by using the sourceArc's
     * payload as a key to encrypt the destArc's payload.
     * This operation can be invoked on both source and sink TAs.
     * Both the input arcs must be per-boot key based arcs.
     *
     * Before creating the new arc, the TA performs a series of checks to verify
     * that the creation should be allowed. If any check fails, the instance
     * must return an ENFORCEMENTS_FAILED service specific error is returned.
     *
     * Perform following steps with respect to parentArc and childArc:
     * 1. CHECK that sourceArc and destArc are successfully decrypted using the
     *    per-boot key of TA and AES GCM decryption. Successful decryption means
     *    that both the arc's are valid and owned by the TA. Extract the key from
     *    sourceArc's payload (source payload key).
     * 2. CHECK that destArc has MintingAllowed policy defined and parentArc has
     *    at least sourceId or sinkId defined.
     * 3. CHECK that destArc's mintingAllowed sourceIds and sinkId policies
     *    are supported by sourceId and sinkId identity's dice certificate chain
     *    in the sourceArc's permission.
     * 4. CHECK that those supported policies from destArc matches the sourceId and sinkId
     *    identity's dice policy from sourceArc. If they do not match then then return
     *    service specific response KEY_ROTATION_REQUIRED.
     * 5. CHECK that if sinkId policy is defined in the sourceArc's MintingAllowed
     *    permission then it is equal to and destArc's sinkId policy.
     * 6. CHECK if the UIDPolicy in sourceArc MintingAllowed permission is Multiple
     *    is equal to that in destArc.
     * 7. CHECK if the UID Policy is destArc is Single and the permissions in both
     *    destArc and sourceArc specifies UID then they must be equal.
     *
     * Next, create new arc content as follows:
     *  1. Copy the permissions and limitations from destinition arc to new arc.
     *  2. Append UIDs from sourceArc if the UIDPolicy of the new arc is Multiple.
     *  3. Copy the payload from the destArc to new arc.
     *
     * Finally create the new arc by using source payload key to encrypt the payload
     * of the dest Arc using AES GCM encryption.
     *
     * @param sourceArc - this is the arc which is encrypted by the per-boot key
     * @param destArc - this is the arc which is also encrypted by the per-boot key.
     * @return the newly created arc payload is from destArc and is encrypted by
     *         the payload key from the sourceArc.
     */
    android.hardware.security.authgraph.Arc mint(
            in android.hardware.security.authgraph.Arc sourceArc,
            in android.hardware.security.authgraph.Arc destArc);
}
