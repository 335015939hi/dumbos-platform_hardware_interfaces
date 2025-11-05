/*
 * Copyright (C) 2025 The Android Open Source Project
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

import android.hardware.contexthub.IUnifiedMotionContextCallback;
import android.hardware.contexthub.MotionSubscription;

@VintfStability
interface IUnifiedMotionContext {
    /**
     * API for the client to request a client ID. This ID will be used to associate the client with
     * motion subscriptions and callbacks. This should only be called oncer per client instance,
     * with the value saved and used in all subsequent calls to the Unified Motion Context service.
     *
     * @return The client ID.
     */
    int requestClientId()

    /**
      * API for the client to register callbacks to receive motion events and information
      * about the motion context service. This must be called prior to
      * registerMotionSubscriptions for proper delivery of motion events.
      *
      * @param callback The callback to receive motion events.
      *
      * TODO: Should we merge this with requestClientId?
      */
    void registerCallback(int clientId, in IUnifiedMotionContextCallback callback);

    /**
     * API for the client to register motion subscriptions. When subscribed,
     * the client will receive MotionEvents that fit the criteria detailed by the subscritions.
     *
     * When called, any existing subscriptions for this client will be removed.
     * An empty list will clear all subscriptions.
     *
     * @param clientId The identifier for the requesting client. This should be obtained by
     *         requestClientId() and used in all cases associated with this client.
     * @param subscriptions A list of all motion subscriptions to register.
     *
     * TODO: Would it be better to separate the callback and motion subscriptions?
     *       +/ roll the callback into the MotionSubscription?
     */
    void registerMotionSubscription(int clientId, in MotionSubscription[] subscriptions);

    /**
     * API for the client to unregister motion subscriptions. Functionally identical to calling
     * registerMotionSubscription with an empty list of subscriptions.
     *
     * @param clientId The identifier for the requesting client. This should be obtained by
     *         requestClientId() and used in all cases associated with this client.
     */
    void unregisterMotionSubscription(int clientId);

    // TODO: Do we need some sort of query for versioning/ capabilities?
}