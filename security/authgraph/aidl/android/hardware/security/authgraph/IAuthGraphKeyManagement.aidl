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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.security.authgraph;

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.Capability;
import android.hardware.security.authgraph.Role;

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
 *              i. a shared key setup by executing the authenticated key exchange protocol defined
 *                 in `IAuthGraphKeyExchange` API.
 *             ii. a long term encryption key owned by each party
 *            iii. a symmetric encryption key kept in memory with per-boot life time of the
 *                 participant (a.k.a per-boot key)
 *     2. A user who wants to create a secret at the sink, that is protected by a key held by the
 *        source, should have setup a key that is unlocked by user authentication at the source
 *        (called "cred key"), via a source specific functionality. The cred key is associated with
 *        the user via a unique identifier (called UID).
 *
 * This protocol, combined with the protocol defined in `IAuthGraphKeyExchange` API, provides the
 * following security guarantees:
 *     1. Any compromised party via whom the source and sink communicate, cannot trick the sink into
 *        creating a user secret that is encrypted with a non-legitimate key, by pretending that it
 *        is issued by the genuine source (during creation of the protected secrets, it is assumed
 *        that both the source and sink themselves are not compromised).
 *     2. Even if the sink is compromised later on, the sink cannot unlock the protected user
 *        secrets created at the sink, as long as the sink does not have access to the source's key.
 *     3. Any component other than the source who created the original key that was used to protect
 *        the user's resource created at the sink, cannot help sink to unlock that key.
 *
 * ErrorCodes are defined in android.hardware.security.authgraph.ErrorCode.aidl.
 * @hide
 */
/* @hide */
@VintfStability
interface IAuthGraphKeyManagement {
    /*
     * This operation creates a new secret key and returns a persistent arc
     * where this key is wrapped by the payload key from the wrappingArc with the
     * capabilities specified in the input caps.
     * This operation can be invoked at TA which is either source, sink or both.
     * The input wrappingArc to this operation must be encrypted by the
     * per-boot key.
     *
     * Perform the following steps with respect to input parameter role:
     * 1. CHECK whether role is supported by this TA.
     *
     * Perform the following steps with respect to input parameter wrappingArc:
     *  1. If this is a source TA then CHECK if the sourceId in the wrappinArc is
     *     supported by the TA. If this is a sink TA then CHECK if the sinkId in
     *     the wrappinArc is supported by the TA.
     *  2. CHECK if the payloadType is secretKey.
     *  3. CHECK if MintineAllowed permission is defined then source Ids, sinIds
     *     and UIDPolicy is allowed by the TA.
     *  4. CHECK and Un-wrap the wrappingArc using per-boot key.
     *  5. CHECK that decrypted payload is AES GCM summetric key
     *  6. Extract the key from the payload.
     *
     * If input parameter childArcCaps is provided then perform the following steps:
     *  1. If the permission is defined in childArcCaps then if the "MintingAllowed"
     *     is specified in childArcCaps:
     *          - If provided CHECK if the requested sourceIds, the sink Ids and
     *              the UIDPolicy are supported in the TA.
     *          - If wrappingArc has "MintingAllowed" permission:
     *              1. If both wrappigArc and cildArcCaps "MintingAllowed" contains
     *                 SinkID then CHECK that they are equal.
     *              2. If wrappingArc contains UIDPolicy = multiple then childArc's
     *                 policy cannot be single.
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
     * If the input parameter childArcCaps is not provided then copy the
     * protected header from wrappingArc to new arc.
     *
     * Perform the following steps for creating and returning a new persistent arc:
     *      1. Generate protected header as follows
     *            - If the input parameter childArcCaps is not provided then
     *               copy the protected header from wrappingArc to new arc.
     *            - Else,
     *            - Copy UIDs from wrappingArc.
     *            - Copy limitations from childArcCaps and then mutually exclusive
     *              limitations from wrappingArc.
     *            - If this is source TA and input parameter role is source
     *              then add Source Id of this TA to the permission.
     *            - If this is sink TA and input parameter role is sink
     *              then add Sink Id of this TA to the permission.
     *            - Generate MintingAlllowed permission by combining "MintingAllowed"
     *              of the wrappingArc (if defined) and adding new permissions
     *              and limitations to it from childArcCaps as follows:
     *               - If "MintingAllowed" permission is defined in wrappingArc then
     *                 add that to new arc.
     *               - If childArcCaps is defined then add that to wrappingArc.
     *               - If the input parameter role is source then add TA's source
     *               Id to "MintingAllowed" if not already present.
     *               - If the input parameter role is sink then add TA's sink
     *               Id to "MintingAllowed" if not already present.
     *               - Add UIDPolicy=single if neither wrappingArc or childArcCaps
     *               specifies it.
     *              - CHECK that the generated "MintingAllowed" does not
     *              have more than one Sink Id.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new persistent arc by encrypting the generated payload with
     *          the wrappingArc's secert key by using AES GCM algorithm.
     *
     * If any of the above CHECK fails then a ENFORCEMENTS_FAILED service specific
     * error is returned.
     *
     * @param wrappingArc - this is the encrypting arc with long term key payload.
     * @param caps - these are set of source ids, sink ids and UIDPolicy.
     * @param role - this is role of the arc i.e. either source or sink.
     * @return the newly created persistent arc.
     */
    android.hardware.security.authgraph.Arc create(
            in android.hardware.security.authgraph.Arc wrappingArc,
            in @nullable android.hardware.security.authgraph.Capability caps,
            in android.hardware.security.authgraph.Role role);

    /*
     * This operation method merges the parentArc with the childArc to generate a
     * new arc by using the parentArc's encrypting key to encrypt the childArc's
     * secret payload. This operation can be invoked on both source and sink TAs.
     * The input parentArc of this operation must be per-boot key based arc and
     * the input childArc must be encrypted by the parentArc.
     *
     * Perform following steps with respect to parentArc and childArc:
     *    1. CHECK that payload type of the parentArc is secret key. The payloadType
     *       for the childArc can be secretKey or Arc.
     *    2. CHECK the source id of parentArc matches the id of the TA if it is
     *       a Source TA. Else sink Id matches the id of the TA if it a Sink TA.
     *    3. For childArc, CHECK if the MintineAllowed permission is defined and do
     *       the following:
     *        - If the identity in parentArc does not match the relevant
     *          identities in the "MintingAllowed" permission in the childArc then
     *          return service specific response KEY_ROTATION_REQUIRED.
     *        - Else CHECK source Ids, sinkIds and UIDPolicy is allowed by the TA.
     *    4. CHECK and decrypt the parentArc using perboot key of the TA.
     *    5. CHECK that payload is a symmteric AES GCM key.
     *    6. Extract the symmteric key from the payload.
     *    7. CHECK and decrypt the childArc using symmteric key from the parentArc.
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
     *         - Copy Source or Sink Ids from parentArc to tbe new arc.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new arc by encrypting the generated payload with
     *          the parentArc's encrypting key by using AES GCM algorithm.
     *
     * The encrypting key of parent arc must be a per-boot key. The payloadType of
     * the parentArc must be a symmteric secret key.
     *
     * If any of the above CHECK fails then a ENFORCEMENTS_FAILED service specific
     * error is returned.
     *
     * @param parentArc - this is the arc which is encrypted by the per-boot key.
     * @param childArc - this is the arc encrypted by shared-key or long-tern
     *                    persistent key which is part of parent arc's payload.
     * @return the newly created arc encrypted by per-boot key contaning payload
                from the childArc.
     */
    android.hardware.security.authgraph.Arc snap(
            in android.hardware.security.authgraph.Arc parentArc,
            in android.hardware.security.authgraph.Arc childArc);
    /*
     * This operation method creates a new persisent arc by using the sourceArc's
     * payload as encrypting key to encrypt the destArc's payload.
     * This operation can be invoked on both source and sink TAs.
     * Both the input arcs must be per-boot key based arcs.
     *
     * Perform following steps with respect to sourceArc and destArc:
     *    1. CHECK that payload type of the parentArc is secret key. The payloadType
     *       for the childArc can be secretKey or Arc.
     *    2. CHECK the source id of parentArc matches the id of the TA if it is
     *       a Source TA. Else sink Id matches the id of the TA if it a Sink TA.
     *    3. For childArc, CHECK if the "MintineAllowed" permission is defined and do
     *       the following:
     *        - If the identity in parentArc does not match the relevant
     *          identities in the "MintingAllowed" permission in the childArc then
     *          return service specific response KEY_ROTATION_REQUIRED.
     *        - Else
                  - CHECK that "MintingAllowed" permission in sourceArc is strictly
                  less than destArc e.g. if sourceArc has SinkId specified and
                  destArc does not have SinkId specified then this operation is
                  not allowed.
                  - CHECK that source Ids, sinkIds and UIDPolicy in destArc is
                    allowed by the TA.
     *    4. CHECK and decrypt the sourceArc and destArc using perboot key of the TA.
     *    5. CHECK that payload of sourceArc is a symmteric AES GCM key.
     *    6. Extract the symmteric key from the payload.
     *
     * Perform the following steps to generate the new arc:
     *    1. Generate protected header as follows:
     *         - Copy the permissions and limitations from destArc to the new arc.
     *         - Copy UIDs from destArc to new arc.
     *         - Copy Source or Sink Ids from destArc to tbe new arc.
     *       2. Generate AES GCM Symmetric Secret Key payload.
     *       3. Generate new arc by encrypting the generated payload with
     *          the sourceArc's encrypting key by using AES GCM algorithm.
     *
     * If any of the above CHECK fails then a ENFORCEMENTS_FAILED service specific
     * error is returned.
     *
     * @param sourceArc - this is the arc which is encrypted by the per-boot key
     * @param destArc - this is the arc which is also encrypted by the per-boot key.
     * @return the newly created arc payload is from destArc and is ecnrypted by
     *         the payload key from the sourceArc.
     */
    android.hardware.security.authgraph.Arc mint(
            in android.hardware.security.authgraph.Arc sourceArc,
            in android.hardware.security.authgraph.Arc destArc);
}
