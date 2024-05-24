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

package android.hardware.security.securetime;

import android.hardware.security.authgraph.Arc;

/**
 * ISecureTime should be implemented in a secure area which has a notion of time. Time is expressed
 * in milliseconds since some arbitrary point in time.  Time must be monotonically increasing,
 * and a secure environment's notion of "current time" must not repeat until the Android device
 * reboots, or until at least 50 million years have elapsed (note that this requirement is satisfied
 * by setting the clock to zero during each boot, and then counting time accurately).
 *
 * Both the authenticators and the consumers of the proofs of authentication issued by the
 * authenticators, who do not have the notion of time, should use the secure time issued by the
 * ISecureTime service.
 *
 * Time is communicated from the ISecureTime service to either of the aforementioned types of
 * consumers of secure time in an authenticated and a secure manner. This is achieved by encrypting
 * the time with a pairwise unique shared key that the ISecureTime service has setup with the
 * recipient by running an authenticated key exchange protocol via IAuthGraphKeExchange API during
 * each boot. The recipient is able to verify the identity of the ISecureTime service during the key
 * exchange.
 *
 * Authenticators attach the authenticated secure time issued by this service in the proof of
 * authentications issued by them. Authenticators do not need to provide a challenge when obtaining
 * the time because an attacker do not gain anything by replaying an old secure time to an
 * authenticator.
 *
 * Consumers of proofs of authentication issued by the authenticators should compare the time
 * included in a proof against the time obtained from this service when deciding whether a given
 * proof of user authentication is sufficiently fresh. They do need to provide a challenge when
 * obtaining the time from this service in order to prevent replay attacks.
 * @hide
 */
@VintfStability
interface ISecureTime {
    /**
     * Get the current time to be shared with the recipient  with whom the `sharedKey` has been
     * setup.
     *
     * @param sharedKey - an arc encrypting the shared key setup with the recipient of the time.
     *
     * @param challenge - an optional challenge value provided by the recipient of the time. It will
     *                    be included in the protected headers of the returned arc to ensure
     *                    freshness. The recipient must ensure that the challenge cannot be
     *                    specified or predicted by an attacker.
     *
     * @return an arc encrypting the current time with the shared key setup with the recipient (i.e.
     *         payload key of the sharedKey arc) which contains the following protected headers:
     *           i) source_id - `Identity` of the ISecureTime service
     *          ii) challenge - the input challenge
     *         iii) PayloadType - Timestamp
     */
    Arc getCurrentTime(in Arc sharedKey, in long challenge);

    /* TODO: add the AuthGraph KE API */
}
