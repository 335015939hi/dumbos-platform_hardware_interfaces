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

import android.hardware.security.see.authmgr.IAuthSession;

/**
 * IAuthMgr provides an interface for authentication of a client to a server. The client is expected
 * to be running in an attested protected VM (pVM). The server, is expected to be running in a more
 * privileged secure environment than the client, such as ARM TrustZone.
 *
 * This protocol is split into two dependent stages.
 *
 * 1) An initial stage executed before a VM launches apps that want to connect to the server.
 *
 * This portion of the protocol is trust-on-first-use, with the goal being that once the service
 * authenticates a VM, no other VM should be able to impersonate that VM to the TEE.
 *
 * 2) Rollback-protected app payload authentication and authorization. App payloads, identified by
 * their own DICE nodes (leaves of the VM DICE chain), are matched against DICE policies that
 * provide rollback protection. The successful completion of this step will result in a connection
 * to the requested Trusted HAL service being passed to the requesting app.
 *
 * Note: This protocol provides no confidentiality. It is up to implementations to ensure that
 * communication occurs over a trusted channel.
 */
interface IAuthMgr {
    const int ERROR_AUTH_FAILURE = 1;
    const int ERROR_INTERNAL = 2;
    const int ERROR_ALLOCATION = 3;

    /**
     * Start a session to authenticate a client to a server, authenticate and authorize client
     * apps, and facilitate connection establishment to trusted services on the server.
     *
     * On failure, a service specific error, defined above, will be returned.
     */
    IAuthSession startSession();
}
