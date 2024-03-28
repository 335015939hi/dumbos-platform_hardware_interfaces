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
 * IAuthMgr provides an interface for mutual authentication between two parties:
 *
 * 1. A client, which is expected to be running in an attested protected VM (pVM).
 *
 * 2. A server, which is expected to be running in a more privileged secure environment than the
 *    client, such as ARM TrustZone.
 *
 * This protocol is split into two dependent stages.
 *
 * 1) An initial stage executed before a VM launches apps that are permissioned to connect to the
 * TEE:
 *
 * This portion of the protocol is trust-on-first-use, with the goal being that once the service
 * authenticates a VM, no other VM should be able to impersonate that VM to the
 * TEE. A VM that expects to use trusted HALs (TODO define) is expected to gate launching apps on
 * the success of mutual authentication between itself and the TEE. If authentication fails, the
 * client must not launch app payloads.
 *
 * 2) A rollback-protected app payload authentication, executed on first attempted use of TEE
 *    services by a client payload. TODO: This portion of the protocol is not yet implemented.
 *
 * Note: This protocol provides no confidentiality. It is up to implementations to ensure that
 * communication occurs over a trusted channel.
 */
interface IAuthMgr {
    const int ERROR_AUTH_FAILURE = 1;
    const int ERROR_INTERNAL = 2;
    const int ERROR_ALLOCATION = 3;

    /**
     * Start a session to mutually authenticate a client and service pair and authorize client
     * payloads.
     *
     * On failure, a service specific error, defined above, will be returned.
     */
    IAuthSession startSession();
}
