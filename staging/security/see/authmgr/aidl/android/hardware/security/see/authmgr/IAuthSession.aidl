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

import android.hardware.security.see.authmgr.AppRequest;
import android.hardware.security.see.authmgr.ClientRequest;
import android.hardware.security.see.authmgr.ClientResponse;
import android.hardware.security.see.authmgr.ServerResponse;

interface IAuthSession {
    /**
     * A client begins mutual authentication with the server by calling `start`.
     *
     * @param ClientRequest containing the client identity
     * @return ServerResponse containing the service's auth challenge.
     */
    ServerResponse start(in ClientRequest request);

    /**
     * Upon receiving a ServerResponse the client validates it and then calls
     * `authenticate` with a ClientResponse. See ClientResponse for details
     * on how servers should validate.
     *
     * After a successful `authenticate` call, a server must bind the VM identity to the VM-ID it
     * observes on the physical connection and reject further communication where:
     * 1) The VM's identity is seen associated with another VM-ID
     * 2) Another VM's identity is seen associated with the same VM-ID
     *
     * This invariant must hold until the server has been notified out of band by a trusted
     * component that the VM-ID is suitable for reuse.
     *
     * TODO enumerate error cases
     */
    void authenticate(in ClientResponse response);

    /**
     * Once a VM is successfully authenticated, it can issue requests to establish a new
     * connection on behalf of apps in the VM to a service in the SEE.
     *
     * An AuthMgr client opens a new connection to the AuthMgr service out of band and associates
     * the out of band connection with the result of this `authorizeApp` call with a unique one-time
     * token.
     *
     * The client then calls authorizeApp, which includes the token in AppRequest. If the client
     * receives a successful response from authorizeApp, the out-of-band connection should be
     * considered authenticated and authorized. The AuthMgr client library and the AuthMgr
     * service can pass on handles for the open connection back to the requesting app and the
     * trusted service, respectively.
     */
    void authorizeApp(in AppRequest appRequest);
}
