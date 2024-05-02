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

package android.hardware.security.gateweaver;

import android.hardware.security.authgraph.Arc;

/**
 * IGateWeaver (GW) provides unified way to protect user data gated by Lock Screen Knowledge Factor
 * (LSKF) with SE/TEE enforced rate limiting and secure deletion, on all devices.
 *
 * The secure storage of GateWeaver is structured as a two dimensional array of slots as follows:
 * --------------------------------------------------------
 * | slot_id | encryption_key | failure_count | wait_time |
 * --------------------------------------------------------
 * 1) slot_id - index into a particular slot in the array of slots.
 * 2) encryption_key - a securely deletable key that is used to provide the outer encryption layer
 *                     of the GW blob associated with the slot.
 * 3) failure_count - the number of consecutive failed verification attempts on the GW blob
 *                    associated with the slot
 * 4) wait_time - time (in milliseconds) to wait until the next verification attempt is allowed on
 *                the GW blob associated with the slot. Wait time is defined by the ratelimiting
 *                schedule. A ratelimiting schedule is an array indexed by the failure count.
 *
 * AuthGraph enabled GateWeaver provides two artifacts:
 * 1) a unique cryptographic key for encrypting LSKF-gated user data,
 * 2) a proof of user authentication, and makes them available to any component which need them.
 * @hide
 */
@VintfStability
@SensitiveData
interface IGateWeaver {
    /**
     * Create a random cryptographic key (K_cred). K_cred is protected with two layers of
     * encryption. First, it is encrypted with the protector-key. The output blob of the first
     * encryption is then encrypted with the per-slot encryption key associated with the given
     * slotID. Return the GW blob which protects K_cred with these two layers of encryption
     *
     * Steps:
     * 1) Create K_cred - a 256-bit cryptographic key for AES-GCM encryption
     * 2) Create an arc encrypting K_cred with the `protector_key`
     * 3) Create an arc encrypting the arc from step 2 with the per-slot encryption key associated
     * with the given slotID and add the following protected headers:
     *     i) UID - a randomly generated unique identifier for the GW blob
     *    ii) slot_id - the slot id given as input
     *   iii) ratelimiting_schedule - the scheduleID given as input
     *
     * @param slotID - index in the array of slots representing the GateWeaver storage, that
     *                 corresponds to the GW blob output by this method call.
     *
     * @param scheduleID - the ratelimiting schedule to be used during verification. If a
     *                     verification attempt fails, GW records the number of consecutive failure
     *                     attempts and the wait time enforced until the next verification attempt
     *                     is allowed, associated with the slotID. The wait time until next
     *                     verification attempt is retrieved from the ratelimiting schedule given
     *                     the number of failed verification attempts.
     *
     * @return the GW blob
     */
    Arc create(in int slotID, in int scheduleID, in byte[] protector_key);

    /**
     * Decrypt K_cred from the `gateweaverBlob` and return an `Arc` encrypting K_cred with the
     * per-boot key of the GW. This returned arc is intended to be used by the
     * AuthGraphKeyManagement API implementation of the GW.
     *
     * K_cred can be recovered only if:
     * 1) the per-slot encryption key associated with the slot id of the GW blob is not reset
     * 2) the same protector-secret key that was provided when creating K_cred is provided as an
     * input to “verify”.
     * GW applies SE or TEE based rate limiting on failed verification attempts, based on the
     * ratelimiting schedule that was specified during creation of the GW blob.
     *
     * @param gateWeaverBlob - a GW blob returned by a previous call to `create`.
     *
     */
    Arc verify(in Arc gateWeaverBlob, in byte[] protector_key, in long challenge, in Arc timestamp);

    void invalidate(in int slotID);

    ReEnrollResponse reEnroll()
}
