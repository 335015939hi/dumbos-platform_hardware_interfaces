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
 *             ii. a long-term encryption key owned by each party
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
     * This method creates a new secret key and returns an arc where this key
     * is wrapped by the payload key from the wrappingArc with the
     * capabilities specified in the input childArcCaps.
     *
     * The childArcCaps is a set of attributes which specifies permissions
     * and limitations. These permissions and limitations constrains the usage
     * of the arc e.g. how the arc can be used and by which actors. Permission
     * is a positive statement about the encryption/decryption key K of the arc
     * and its characteristics about what that key can be used for.
     * The limitation is a negative statement about the arc's key K such as
     * expiry time after which key must not be used, maximum usage count of the
     * key, etc.
     *
     * This operation can be invoked at the TA which is either source or sink.
     * The input wrappingArc to this operation must be encrypted by the per-boot
     * key.
     *
     * Before creating the new arc, the TA performs
     * a series of checks to verify that the creation should be allowed. If
     * any check fails, the TA must return an ENFORCEMENTS_FAILED service
     * specific error is returned.
     *
     * Perform the following steps with respect to input parameter role:
     * 1. CHECK whether role is supported by TA.
     *
     * Perform the following steps with respect to input parameter wrappingArc:
     *  1. If the TA is a source then CHECK if the sourceId in the wrappingArc
     *     is present in its allowedSources data or if the TA is a sink then
     *     CHECK if the sinkId in the wrappingArc is present in its allowedSinks
     *     data.
     *  2. CHECK if the payloadType is a secretKey.
     *  3. CHECK if "MintingAllowed" permission is defined then sourceIds, sinkIds
     *     and UIDPolicy are allowed by the TA.
     *  4. Decrypt the wrappingArc using per-boot key and CHECK if that happens
     *     successfully.
     *  5. CHECK that decrypted payload is AES GCM symmetric key.
     *  6. Extract the key from the payload.
     *
     * If input parameter childArcCaps is provided then perform the following steps:
     *  1. If the permission is defined in childArcCaps and if the "MintingAllowed"
     *     is specified in childArcCaps:
     *          - If provided CHECK if the requested sourceIds, sinkIds and
     *              UIDPolicy are in the TA.
     *          - If wrappingArc has "MintingAllowed" permission:
     *              1. If "MintingAllowed" permission in both the wrappingArc and
     *                 the childArcCaps  contains SinkId then CHECK that they are equal.
     *              2. If wrappingArc contains UIDPolicy = multiple then CHECK that
     *                 childArcCaps's UIDPolicy, if specified, cannot be single.
     *  2. If there is limitation defined in wrappingArc and in childArcCaps
     *    then CHECK that limitations of the childArcCaps is greater than or equal
     *    to that in wrappingArc.
     *       - CHECK that if challenge is defined in wrappingArc no challenge is
     *         included in childArcCaps.
     *       - CHECK that if timeout constraint is defined in both wrappingArc and
     *         childArcCaps then the one in later is less than or equal to the
     *         former.
     *       - CHECK that if maxUsageCount constraint is defined in both
     *         wrappingArc and childArcCaps then the one in later is less than or
     *         equal to the former.
     *
     * Perform the following steps for creating and returning a new arc:
     *      1. Generate protected header as follows
     *            - If the input parameter childArcCaps is not provided then
     *               copy the protected header from wrappingArc to new arc.
     *            - Else,
     *              - Copy UIDs from wrappingArc.
     *              - Copy limitations from childArcCaps and then mutually exclusive
     *                limitations from wrappingArc.
     *              - If the TA is source and the input parameter role = source then add
     *                sourceId of the TA to the permission of the new arc.
     *              - If the TA is sink and the input parameter role = sink then add SinkId
     *                of the TA to the permission of the new arc..
     *              - Generate "MintingAllowed" permission of the new arc as follows:
     *                - Combine "MintingAllowed" permission of the wrappingArc (if defined) and
     *                  "MintingAllowed" permission from childArcCaps.
     *                - If the input parameter role = source then add TA's sourceId else
     *                  if the input parameter role = sink then add TA's sink Id.
     *                - Add UIDPolicy=single if neither wrappingArc or childArcCaps specifies it.
     *                - CHECK that the new arc's "MintingAllowed" permission does not have more
     *                  than one SinkId.
     *              - Add limitations to the new arc by combining those in the wrappingArc and in
     *                the childArcCaps.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new arc by encrypting the generated payload with
     *          the wrappingArc's secret key by using AES GCM algorithm.
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
     * secret payload. This operation can be invoked on both source and sink TAs.
     * The parentArc's encrypting key must be a per-boot key and the childArc
     * must be encrypted by the parentArc's payload key.
     *
     * Before creating the new Arc, the TA performs a series of checks to
     * verify that the creation should be allowed. If any check fails, the TA must
     * return an ENFORCEMENTS_FAILED service specific error is returned.
     *
     * Perform following steps with respect to parentArc and childArc:
     *    1. CHECK that payload type of the parentArc is secret key. The payloadType
     *       for the childArc can be secretKey or Arc.
     *    2. CHECK the sourceId of parentArc matches the id of the TA if it is a
     *       source. Else CHECK sinkId matches the id of the TA if it is a sink.
     *    3. For childArc, CHECK if the "MintingAllowed" permission is defined and do
     *       the following:
     *        - If the identity in parentArc does not match the relevant
     *          identities in the "MintingAllowed" permission in the childArc then
     *          return service specific response KEY_ROTATION_REQUIRED.
     *        - Else CHECK sourceIds, sinkIds and UIDPolicy is allowed by the TA.
     *    4. Decrypt the parentArc using per-boot key of the TA and CHECK if that happens
     *       successfully.
     *    5. CHECK that payload is a symmetric AES GCM key.
     *    6. Extract the symmetric key from the payload.
     *    7. CHECK and decrypt the childArc using symmetric key from the parentArc.
     *
     * Perform the following steps to generate the new arc:
     *    1. Generate protected header as follows:
     *         - Copy the permissions from childArc to the new arc.
     *         - Copy the limitations from both parentArc and childArc to the new
     *           arc such that resulting limitation set is equal to or more than
     *           both the parent and childArc. e.g. if the parentArc's timeout is 10
     *           minutes and childArc's timeout is 5 minutes then new arc will have
     *           timeout of 5 minutes.
     *           If only one of the arc has challenge limitation then copy that to
     *           the new arc. If both the arcs have challenge limitations then copy
     *           the childArc's challenge to the new arc.
     *         - Copy UIDs from childArc to new arc.
     *         - Copy Source or sinkIds from parentArc to tbe new arc.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new arc by encrypting the generated payload with
     *          the parentArc's encrypting key by using AES GCM algorithm.
     *
     * The encrypting key of parent arc must be a per-boot key. The payloadType of
     * the parentArc must be a symmetric secret key.
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
     * Perform following steps with respect to sourceArc and destArc:
     *    1. CHECK that payload type of the parentArc is a secretKey. The payloadType
     *       for the childArc can be the secretKey or the Arc.
     *    2. CHECK the sourceId of parentArc matches the id of the TA if it is
     *       a Source TA. Else sinkId matches the id of the TA if it a Sink TA.
     *    3. For childArc, CHECK if the "MintingAllowed" permission is defined and do
     *       the following:
     *        - If the identity in parentArc does not match the relevant
     *          identities in the "MintingAllowed" permission in the childArc then
     *          return service specific response KEY_ROTATION_REQUIRED.
     *        - Else
     *            - CHECK that "MintingAllowed" permission in sourceArc is strictly
     *            less than destArc e.g. if sourceArc has SinkId specified and
     *            destArc does not have SinkId specified then this operation is
     *            not allowed.
     *            - CHECK that sourceIds, sinkIds and UIDPolicy in destArc is
     *              allowed by the TA.
     *    4. Decrypt the sourceArc and destArc using the per-boot key of the TA and
     *       CHECK if that happens successfully.
     *    5. CHECK that payload of sourceArc is a symmetric AES GCM key.
     *    6. Extract the symmetric key from the payload.
     *
     * Perform the following steps to generate the new arc:
     *    1. Generate protected header as follows:
     *         - Copy the permissions and limitations from destArc to the new arc.
     *         - Copy UIDs from destArc to new arc.
     *         - Copy Source or sinkIds from destArc to tbe new arc.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new arc by encrypting the generated payload with
     *          the sourceArc's encrypting key by using AES GCM algorithm.
     *
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
