/*
 * Copyright 2024 The Android Open Source Project
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

import android.hardware.security.see.authmgr.ClientAuthRequest;
import android.hardware.security.see.authmgr.SignedClientResponse;
import android.hardware.security.see.authmgr.SignedServerResponse;

/**
 * ITrustedHalAuthManager provides an interface for mutual authentication between two parties:
 *
 * 1. A client, which is expected to be running in an attested protected VM (pVM) in process that is
 *    more privileged than the apps it will eventually enable to communicate with the TEE.
 *
 * 2. A server, which is expected to be running in a more privileged secure environment than the
 *    client, such as ARM TrustZone.
 *
 * This protocol is split into two dependent stages.
 *
 * 1) An initial stage executed on client boot:
 *
 * This portion of the protocol is trust-on-first-use, with the goal being that once a server
 * authenticates a client, no other VM should be able to impersonate that client to the
 * server. A client that expects to use trusted HALs is expected to gate booting on the success
 * of mutual authentication between itself and the server. If authentication fails, the client
 * must not launch app payloads.
 *
 * 2) A rollback-protected app payload authentication, executed on first attempted use of TEE
 *    services by a client payload. TODO: This portion of the protocol is not yet implemented.
 *
 * Note: This protocol provides no confidentiality. It is up to implementations to ensure that
 * communication occurs over a trusted channel.
 */
interface ITrustedHalAuthMgr {
    const int ERROR_AUTH_FAILURE = 1;
    const int ERROR_INTERNAL_ERROR = 2;

    /**
     * A client begins mutual authentication with the server by calling `issueClientChallenge`.
     *
     * If this call returns an error, authentication has failed and both the client and the server
     * should permanently clean up state associated with this authentication attempt.
     *
     * TODO enumerate error cases.
     *
     * @param ClientAuthRequest containing both the client identity and an auth challenge for the
     *         client
     *        to authenticate the server.
     * @return A SignedServerResponse containing the server challenge response and the server
     *         challenge to authenticate the client.
     */
    SignedServerResponse issueClientChallenge(in ClientAuthRequest request);

    /**
     * Upon receiving a SignedServerResponse the client validates it and then calls
     * `sendSignedClientResponse` with a SignedClientResponse. See SignedServerResponse for details
     * on client validation.
     *
     * See SignedClientResponse for details on server validation.
     *
     * On success, a server will associate the VM-ID of the client with the token it provided in
     * SignedServerResponse. After a successful call, a server must not accept any authentication
     * requests for this VM-ID until it has been notified out of band that the VM has been destroyed
     * and the VM-ID is suitable for reuse.
     *
     * On failure, both the client and the server should clean up all resources including tokens
     * issued.
     *
     * Potential service-specific errors:
     *      1. ERROR_AUTH_FAILURE The server has failed to authenticate the client.
     *      2. ERROR_INTERNAL_ERROR
     */
    void sendSignedClientResponse(in SignedClientResponse response);
}
