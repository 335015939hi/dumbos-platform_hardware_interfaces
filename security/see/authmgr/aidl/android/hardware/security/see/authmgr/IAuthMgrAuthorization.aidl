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
 * authorize one or more clients trusted applets (TA) in the pVM instance to access the trusted
 * services (i.e. Trusted HALs).
 *
 * The AuthMgr authorization protocol consists of two phases:
 *     1. Phase 1 authenticates the AuthMgr FE to the AuthMgr BE. Phase 1 consists of the first two
 *        methods (`initAuthentication` and `completeAuthentication`). At the end of the successful
 *        execution of the first two methods, the AuthMgr FE establishes an authenticated connection
 *        with the AuthMgr BE, with rollback protection.
 *        AuthMgr FE uses this authenticated connection throughout the boot-cycle of the pVM to send
 *        phase 2 requests to the AuthMgr BE. Therefore, phase 1 needs to be executed only once per
 *        boot cycle of the pVM.
 *
 *     2. Phase 2 authorizes a client in the pVM to access trusted service in the TEE and
 *        establishes a new connection between the client and the trusted service so that the client
 *        and the trusted service can communicate independently from the AuthMgr after the execution
 *        of the authorization protocol.
 *        The AuthMgr FE first creates a new vsock connection to the AuthMgr BE and sends a one-time
 *        token over that connection. The AuthMgr FE then invokes the third method of this API
 *        (`getTrustedServiceForClient`) on the authenticated connection established with
 *        the AuthMgr BE in phase 1, in order to authorize a client TA to access a trusted service
 *        and to authorize and delegate the aforementioned new connection for the subsequent
 *        communication between the client and the trusted service. Therefore, the same token sent
 *        over the new connection is sent as an input to `getTrustedServiceForClient` as well.
 *
 * Note that this interface defines a one-way authentication protocol from the AuthMgr FE to AuthMgr
 * BE. This does not cover mutual authentication.
 *
 * The AuthMgr BE should store a `global sequence number` in the secure storage that does not get
 * wiped upon factory reset. This sequence number is used to build a unique identifier for each
 * client of the trusted services in order to prevent the use-after-destroy threat. This means that
 * even if a client is created with the same identifier(s) of a deleted client, the new client
 * should not be able to access the deleted client's secrets.
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
 * service, creating secure storage file paths by the SecureStorage service) in order to prevent
 * aforementioned use-after-destroy threat.
 */
@VintfStability
interface IAuthMgrAuthorization {
    /**
     * AuthMgr FE initiates the challenge-response protocol with the AuthMgr BE in order to
     * authenticate the AuthMgr FE to the AuthMgr BE. AuthMgr BE creates and returns a challenge
     * (a cryptographic random of 32 bytes) to the AuthMgr FE. The AuthMgr BE usually extracts the
     * instance identifier from the DICE certificate chain of the AuthMgr FE, sent as input in
     * `diceCertChain`. If the instance identifier is not included in the DICE certificate chain,
     * then it can be sent in the optional `instanceIdentifier` input parameter. However, such
     * instance identifiers should be known to the AuthMgr BE in an out-of-band mechanism.
     *
     * The instance identifier is used by the AuthMgr BE to detect and reject any duplicate
     * authentication attempts for the same instance ID (from the same pVM or a different pVM with
     * the same instance ID). The error code `INSTANCE_ALREADY_AUTHENTICATED` should be returned in
     * this case.
     *
     * @param diceCertChain - DICE certificate chain of the AuthMgr FE.
     *
     * @param instanceIdentifier - optional parameter to send the instance identifier if it is not
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
     * AuthMgr FE in the DICE certificate chain. The error code `SIGNATURE_VERIFICATION_FAILED`
     * should be returned if the signature verification fails.
     * As per the CDDL for `ResponseForChallenge` in SignedResponseForChallenge.cddl, the AuthMgr FE
     * includes the challenge sent by the AuthMgr BE and the unique transport IDs of the AuthMgr FE
     * and AuthMgr BE. Although it is sufficient to include either of the transport IDs, we specify
     * to include both for the completenss.
     *
     * AuthMgr BE validates the DICE certificate chain by verifying all the signatures in the chain
     * and by checking wither the root public key is trusted. The error code
     * `INVALID_DICE_CERT_CHAIN` should be returned if this fails.
     *
     * During the first invocation of this method in the lifetime of the AuthMgr FE, the AuthMgr BE
     * matches the DICE certificate chain of the AuthMgr FE to the DICE policy given in
     * `dicePolicy`. An error code `DICE_POLICY_MATCHING_FAILED` should be returned if this fails.
     * If the DICE chain to policy matching succeeds, the AuthMgr BE should store the DICE policy in
     * a file path constructed with the instance identifier.
     *
     * In the subsequent invocations of this method, the AuthMgr BE first matches the DICE
     * certificate chain with the stored DICE policy, and if it succeeds, checks if the given
     * DICE poliy is different from the stored DICE policy. If that is the case, the AuthMgr BE
     * matches the DICE certificate chain with the given DICE policy. If it succeeds, the AuthMgr BE
     * replaces the stored DICE policy with the given DICE policy.
     *
     * Upon successful execution of this method, the AuthM BE should mark the connection with the
     * AuthMgr FE as "authenticated" in a some state associated with the connection, in order to
     * distinguish authenicated connections from any non-authenticated connections, so that the
     * requests for phase 2 are allowed only on authenticated connections.
     */
    void completeAuthentication(
            in SignedResponseForChallenge signedResponseForChallenge, in DicePolicy dicePolicy);

    void getTrustedServiceForClient(in byte[] clientID, String serviceName, in byte[32] token,
            in ClientDiceArtifactsForPhase2 clientDiceArtifacts);

    void deleteClient(in byte[] clientID);
}
