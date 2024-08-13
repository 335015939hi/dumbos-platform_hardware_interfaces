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

package android.hardware.security.see.authmgr;

import android.hardware.security.see.authmgr.ClientDiceArtifactsForPhase2;
import android.hardware.security.see.authmgr.DicePolicy;
import android.hardware.security.see.authmgr.ExplicitKeyDiceCertChain;
import android.hardware.security.see.authmgr.SignedResponseForChallenge;

/**
 * This is the interface to be implemented by an AuthMgr backend component (AuthMgr BE), in order to
 * allow AuthMgr frontend components (AuthMgr FE) in a pVM instance to authenticate themselves and
 * to authorize one or more clients in the pVM instance to access the trusted services.
 *
 * The following assumptions must be true for the underlying IPC mechanism and the transport layer:
 *     1. Both parties should be able to retrieve a non-spoofable identifier of the other party from
 *        the transport layer (a.k.a transport ID or vM ID), which stays the same throughout a given
 *        boot cycle of a pVM instance. This is important to prevent person-in-the-middle (PITM)
 *        attacks and to authorize a new connection from a pVM instance based on an already
 *        authenicated connection from the same pVM instance.
 *
 *     2. The platforms where both AuthMgr FE and the AuthMgr BE run, should support handing over a
 *        connection that is setup between them to another party so that such connection can be
 *        delegated for communication between the two new parties. This is important to establish a
 *        new connection between the AuthMgr FE and the AuthMgr BE and then delegate it for the
 *        communication between a client and a trusted service, once the authorization checks on the
 *        client pass.
 *
 *     3. This API should be exposed over an IPC mechanism that support statefull connections. This
 *        is important for the AuthMgr FE to setup an authenicated connection once per boot and
 *        reuse it to authorize multiple client connections.
 *
 *
 * The AuthMgr authorization protocol consists of two phases:
 *     1. Phase 1 authenticates the AuthMgr FE to the AuthMgr BE via the first two methods:
 *        `initAuthentication` and `completeAuthentication`. At the end of the successful excecution
 *        of phase 1, the AuthMgr FE establishes an authenticated connection with the AuthMgr BE.
 *        Phase also provides rollback protection for AuthMgr FE.
 *        Authentication is performed by verifying the AuthMgr FE's signature on the challenge
 *        issued by the AuthMgr BE. The public signing key of the AuthMgr FE is obtained from the
 *        validated DICE certificate chain for verifying the signature. Rollback protection is
 *        achieved by matching the DICE certificate chain against the stored DICE policy.
 *        AuthMgr FE uses this authenticated connection throughout the boot-cycle of the pVM to send
 *        phase 2 requests to the AuthMgr BE. Therefore, phase 1 needs to be executed only once per
 *        boot cycle of the pVM.
 *        AuthMgr BE should take measures to prevent any duplicate authentication attempts from the
 *        same instance or from any impersonating instances.
 *
 *     2. Phase 2 authorizes a client in the pVM to access trusted service in the TEE and
 *        establishes a new connection between the client and the trusted service so that the client
 *        and the trusted service can communicate independently from the AuthMgr after the execution
 *        of the authorization protocol.
 *        The AuthMgr FE first creates a new vsock connection to the AuthMgr BE and sends a one-time
 *        token over that connection. The AuthMgr FE then invokes the third method of this API
 *        (`getTrustedServiceForClient`) on the authenticated connection established with
 *        the AuthMgr BE in phase 1, in order to authorize a client TA to access a trusted service
 *        and to delegate the aforementioned new connection for the subsequent communication between
 *        the client and the trusted service. Therefore, the same token that was sent over the new
 *        connection is sent as an input to `getTrustedServiceForClient`.
 *
 * Note that this interface defines a one-way authentication protocol from the AuthMgr FE to AuthMgr
 * BE. This does not cover mutual authentication.
 *
 * The AuthMgr BE should store a `global sequence number` in the secure storage that does not get
 * wiped upon factory reset. This sequence number is used to build: 1) unique file paths in the
 * the AuthMgr BE's secure storage for the remote clients, 2) a unique identifier to introduce each
 * client to the trusted services with, in order to prevent the "use-after-destroy" threat.
 * This means that even if a client is created with the same identifier(s) of a deleted client, the
 * new client should not be able to access the deleted client's secrets/resources.
 *
 * Everytime the AuthMgr BE sees a new instance or a client, it assigns the current `global sequence
 * number` as the unique sequence number of the instance or the client and increments the `global
 * sequence number`.
 *
 * A client's unique identifier is the combination of the instance's sequence number and the
 * client's sequence number. This unique client identifier is communicated to the trusted service(s)
 * by the AuthMgr BE when an authorized connection is setup between the client and the trusted
 * service in phase 2. The trusted service(s) should mix in this unique client identifier when
 * providing the critical services to the clients (e.g. deriving HW-backed keys by the HWCrypto
 * service, creating secure storage file paths by the SecureStorage service) in order to prevent the
 * aforementioned "use-after-destroy" threat.
 */
@VintfStability
interface IAuthMgrAuthorization {
    /**
     * AuthMgr FE initiates the challenge-response protocol with the AuthMgr BE in order to
     * authenticate the AuthMgr FE to the AuthMgr BE. AuthMgr BE creates and returns a challenge
     * (a cryptographic random of 32 bytes) to the AuthMgr FE. The AuthMgr BE usually extracts the
     * instance identifier from the DICE certificate chain (`diceCertChain`) of the AuthMgr FE.
     * If the instance identifier is not included in the DICE certificate chain, then it should be
     * sent in the optional `instanceIdentifier`. The instance identifiers that are not included in
     * the DICE certificate chain should be known to the AuthMgr BE in an out-of-band mechanism.
     *
     * The instance identifier is used by the AuthMgr BE in this step to detect and reject any
     * duplicate authentication attempts for an instance ID that is already authenticated. The error
     * code `INSTANCE_ALREADY_AUTHENTICATED` should be returned in this case.
     *
     * If authentication is already started (but not completed) from the same transport ID, return
     * the error code `AUTHENTICATION_ALREADY_STARTED`.
     *
     * @param diceCertChain - DICE certificate chain of the AuthMgr FE.
     *
     * @param instanceIdentifier - optional parameter to send the instance identifier, if it is not
     *                             included in the DICE certificate chain
     *
     * @return challenge to be included in the signed response sent by the AuthMgr FE in
     *         `completeAuthentication`
     *
     */
    byte[32] initAuthentication(in ExplicitKeyDiceCertChain diceCertChain,
            in @nullable byte[64] instanceIdentifier);

    /**
     * AuthMgr FE invokes this method to complete phase 1 of the authorization protocol. The AuthMgr
     * BE verifies the signature in `signedResponseForChallenge` with the public signing key of the
     * AuthMgr FE obtained from the DICE certificate chain. The error code
     * `SIGNATURE_VERIFICATION_FAILED` should be returned if the signature verification fails.
     * As per the CDDL for `ResponseForChallenge` in SignedResponseForChallenge.cddl, the AuthMgr FE
     * includes the challenge sent by the AuthMgr BE and the unique transport IDs of the AuthMgr FE
     * and AuthMgr BE. Although it is sufficient to include either of the transport IDs to prevent
     * replay attacks when there more than one AuthMgr BE, where one AuthMgr BE may impersonate an
     * instance/AuthMgr FE to another AuthMgr BE, we specify to include both for the completenss.
     *
     * AuthMgr BE validates the DICE certificate chain by verifying all the signatures in the chain
     * and by checking wither the root public key is trusted. The error code
     * `INVALID_DICE_CERT_CHAIN` should be returned if this fails.
     *
     * During the first invocation of this method in the lifetime of the AuthMgr FE, the AuthMgr BE
     * matches the DICE certificate chain of the AuthMgr FE to the DICE policy given in
     * `dicePolicy`. The error code `DICE_POLICY_MATCHING_FAILED` should be returned if this fails.
     * If the DICE chain to policy matching succeeds, the AuthMgr BE should store the DICE policy in
     * the secure storage via a file path constructed using the instance identifier.
     *
     * In the subsequent invocations of this method, the AuthMgr BE first matches the DICE
     * certificate chain with the stored DICE policy, and if it succeeds, checks if the given
     * DICE poliy is different from the stored DICE policy. If that is the case, the AuthMgr BE
     * matches the DICE certificate chain with the given DICE policy as well. If that succeeds, the
     * AuthMgr BE replaces the stored DICE policy with the given DICE policy.
     *
     * Upon successful execution of this method, the AuthM BE should mark the connection with the
     * AuthMgr FE as "authenticated" in the state associated with the connection, in order to
     * distinguish authenicated connections from any non-authenticated connections. The requests for
     * for phase 2 are allowed only on authenticated connections. Along with the authentication
     * status, the AuthMgr BE may cache other artifacts such as instanceID, transport ID, DICE chain
     * and DICE policy of the AuthMgr FE in the state associated with the connection, so that they
     * can be reused when serving phase 2 requests over the authenticated connection.
     *
     * @param signedResponseForChallenge - signature from AuthMgr FE (data CBOR encoded according to
     *                                     SignedResponseForChallenge.cddl)
     *
     * @param dicePolicy - DICE policy of the AuthMgr FE
     */
    void completeAuthentication(
            in SignedResponseForChallenge signedResponseForChallenge, in DicePolicy dicePolicy);

    /**
     * When the AuthMgr FE receives a request from a client to access a trusted services, the
     * AuthMgr FE first creates a new connection with the AuthMgr BE out-of-band and sends a
     * one-time cryptographic token of 32 bytes over that new connection.
     * The AuthMgr FE then invokes this method on the authenticated connection established with the
     * AuthMgr BE in phase 1. The AuthMgr FE acts as the DICE manager for all the clients in the pVM
     * and generates the DICE leaf certificate and the DICE leaf policy for the client, which are
     * sent as inputs to this function.
     *
     * The AuthMgr BE keeps track of the new connections, the tokens sent over them and the
     * transport ID of the pVM instance which created those connections.
     * When this method is invoked, the AuthMgr BE checks whether the underlying connection of this
     * method call is already authenticated. Otherwise the error code `CONNECTION_NOT_AUTHENTICATED`
     * is returned.
     *
     * The AuthMgr BE first validates the DICE leaf certificate of the client and returns the error
     * code `INVALID_DICE_LEAF` if the validation fails. During the first invocation of this method
     * for a given client, the AuthMgr BE matches the DICE leaf certificate of the client to the
     * client's DICE policy. The error code `DICE_POLICY_MATCHING_FAILED` should be returned if this
     * fails. If the DICE leaf certificate to policy matching succeeds, the AuthMgr BE should store
     * the client's DICE policy in the secure storage via a file path constructed as: instance
     * sequence number + client ID (if the pVM side needs to reuse a client ID once a client is
     * requested to be deleted, then  the client sequence number can be appended to the file path).
     *
     * In the subsequent invocations of this method, the AuthMgr BE first matches the DICE leaf
     * certificate with the stored DICE policy, and if it succeeds, checks if the given
     * DICE poliy is different from the stored DICE policy. If that is the case, the AuthMgr BE
     * matches the DICE leaf certificate with the given DICE policy as well. If that succeeds, the
     * AuthMgr BE replaces the stored DICE policy with the given DICE policy.
     *
     * If the same client requests multiple trusted services or connects to the same trusted service
     * multiple times during the same boot cycle of the pVM instance, it is recommended to validated
     * the client's DICE artifacts only once for a given client as an optimization.
     *
     * Once the validation of the client's DICE artifacts is completed, the AuthMgr BE checks
     * whether there is a pending new connection to be authorized, which is associated with a token
     * that matches the token sent in this method call and a transport ID that matches the transport
     * ID associated with the connection underlying this method call. If there is no such pending
     * connection, return the error code `NO_CONNECTION_TO_AUTHORIZE`.
     *
     * Next the AuthMgr BE connects to the `ITrustedServicesCommonsConnect` interface implemented by
     * the trusted service requested by the client in order to handover the new authorized
     * connection to the trusted service. If this connection handover fails, the error code
     * `CONNECTION_HANDOVER_FAILED` should be returned. Once the connection handover is successful,
     * the AuthMgr BE returns OK to the AuthMgr FE. Then the AuthMgr FE returns to the client a
     * handle to the new connection created at the beginning of phase 2. At this point, an
     * authorized connection is setup between the client and the trusted service, which they can use
     * to communicate independently of the AuthMgr FE and the AuthMgr BE.
     *
     * @param clientID - the identifier of the client in the pVM instance
     *
     * @param service name - the name of the service requested by the client
     *
     * @param token - the one-time token used to authorize the new connection created between the
     *                AuthMgr FE and the AuthMgr BE
     *
     * @param clientDiceArtifactsForPhase2 - DICE leaf certificate and the DICE leaf policy of the
     *                                       client
     */
    void getTrustedServiceForClient(in byte[] clientID, String serviceName, in byte[32] token,
            in ClientDiceArtifactsForPhase2 clientDiceArtifacts);

    /**
     * The AuthMgr FE invokes this method over an authenticated connection setup in phase 1,
     * in order to delete a client while the pVM is still alive.
     *
     * This method is part of the cleanup functionality of the AuthMgr. The implementations may
     * return the service specific error `NOT_IMPLEMENTED` if they do not need to support the
     * deletion of individual clients while the pVM instance is still alive.
     *
     * @param clientID - identifier of the client in the pVM instance
     */
    void deleteClient(in byte[] clientID);
}
