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

import android.hardware.security.see.authmgr.ClientContext;

/**
 * This interface should be implemented by all the trusted services that are available to be
 * accessed by the remote clients.
 * The AuthMgr BE accesses this interface to establish a connection between a remote client and the
 * trusted service, so that the remote client and the trusted service can communicate independently
 * from the AuthMgr, once the AuthMgr authorization protocol is successfully completed.
 */
@VintfStability
interface ITrustedServicesCommonsConnect {
    /**
     * The AuthMgr BE invokes this method to handover an authorized connection to
     * the trusted service. The AuthMgr authorization protocol defined in `IAuthMgrAuthorization`
     * makes sure that the other end of the connection is handed over to the authorized remote
     * client.
     * In addition to the connection handle, the AuthMgr BE also conveys to the trusted service
     * a unique client identifier - which consists of the instance sequence number and the client
     * sequence number, and an optional client context - which consists of the DICE artifacts
     * of the remote client and the persistent state of the remote client.
     * The trusted service should mix in the unique client identifier when providing services to the
     * remote client in order to avoid the "use-after-destroy" threat discused in
     * `IAuthMgrAuthorization`.
     * The trusted services may use the DICE artifacts of the clients (optionally) supplied by the
     * AuthMgr BE to perform any DICE chain to policy matching without relying on the AuthMgr's
     * "DICE chain to policy matching as a service".
     *
     * @param connectionHandle - a handle to the new connection that was setup out-of-band between
     *                           the AuthMgr BE and the AuthMgr FE at the beginning of phase 2 and
     *                           that was authorized during phase 2 of the AuthMgr authorization
     *                           protocol
     * @param instanceSeqNumber - the unique sequence number assigned to the pVM instance in which
     *                            the remote client is running
     * @param clientSeqNumber - the unique sequence number assigned to the client
     * @param clientContext - additional information about the client that maybe useful in providing
     *                        the service, such as the DICE artifacts, persistent state.
     */
    void handOverConnection(in ParcelFileDescriptor connectionHandle, in int instanceSeqNumber,
            in int clientSeqNumber, in @nullable ClientContext clientContext);
}
