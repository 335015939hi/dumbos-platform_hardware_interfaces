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

package android.hardware.contexthub;

import android.hardware.contexthub.EndpointId;
import android.hardware.contexthub.EndpointInfo;
import android.hardware.contexthub.Reason;

@VintfStability
interface IEndpointLifecycleCallback {
    /**
     * Lifecycle event notification for endpoint starting from remote side. There is no need to
     * report already started endpoint prior to the registration of an EndpointLifecycleCallbacks
     * object. The EndpointInfo reported here should be consistent with values from getEndpoints().
     *
     * Endpoints added by registerEndpoint should not be included. registerEndpoint() should not
     * cause this call.
     *
     * @param endpointInfos An array of EndpointInfo representing endpoints that just started.
     */
    void onEndpointStarted(in EndpointInfo[] endpointInfos);

    /**
     * Lifecycle event notification for endpoint stopping from remote side. There is no need to
     * report already stopped endpoint prior to the registration of an EndpointLifecycleCallbacks
     * object. The EndpointId reported here should represent a previously started Endpoint.
     *
     * When a hub crashes or restart, events should be batched into be a single call (containing all
     * the EndpointId that were impacted).
     *
     * Endpoints added by registerEndpoint should not be included. unregisterEndpoint() should not
     * cause this call.
     *
     * @param endpointIds An array of EndpointId representing endpoints that just stopped.
     * @param reason The reason for why the endpoints stopped.
     */
    void onEndpointStopped(in EndpointId[] endpointIds, Reason reason);
}
