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

import android.hardware.security.see.authmgr.ClientRequest;
import android.hardware.security.see.authmgr.ClientResponse;
import android.hardware.security.see.authmgr.ServerResponse;

interface IAuthSession {
    /**
     * A client begins mutual authentication with the server by calling `start`.
     *
     * @param ClientRequest containing the client identity and an auth challenge for the
     *         client to authenticate the service (TODO is this necessary or is the service location
     * trusted?).
     *
     * @return ServerResponse containing the service's auth challenge.
     */
    ServerResponse start(in ClientRequest request);

    /**
     * Upon receiving a ServerResponse the client validates it and then calls
     * `authenticate` with a ClientResponse. See ClientResponse for details
     * on how services should validate.
     *
     * On success, a service will associate the VM-ID of the client with the token it provided in
     * ServerResponse. After a successful `authenticate` call, a service must not accept any
     * authentication requests for this VM-ID until it has been notified out of band that the the
     * VM-ID is suitable for reuse.
     *
     * On failure, both the client and the server should clean up all resources including tokens
     * issued.
     *
     * TODO enumerate error cases
     */
    void authenticate(in ClientResponse response);
}
