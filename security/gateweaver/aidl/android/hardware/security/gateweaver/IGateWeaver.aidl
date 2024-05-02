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
 * IGateWeaver provides unified way to protect user data gated by Lock Screen Knowledge Factor
 * (LSKF) with SE/TEE enforced rate limiting and secure deletion, on all devices.
 *
 * The secure storage of GateWeaver (GW) is structured as a two dimensional array of slots follows:
 * ----------------------------------------------------------------------
 * | slot_id | encryption_key | failure_count | wait_time | re-enrolled |
 * ----------------------------------------------------------------------
 * 1) slot_id - index into a particular slot in the array of slots.
 * 2) encryption_key - a securely deletable key that is used to provide the outer encryption layer
 *                     of the GW blob associated with the slot.
 * 3) failure_count - the number of consecutive failed verification attempts on the GW blob
 *                    associated with the slot
 * 4) wait_time - time (in milliseconds) to wait until the next verification attempt is allowed on
 *                the GW blob associated with the slot. Wait time is defined by the ratelimiting
 *                schedule. A ratelimiting schedule is an array indexed by the failure count.
 * 5) re-enrolled - indicates whether the key (K_cred) encrypted in the GW blob associated with The
 *                  slot is re-enrolled in a new GW blob (via the re-enroll method)
 *
 * TODO: Add a description on the per-factory-reset status that GW maintains about tokens_allowed in
 * order to support tokens based protector keys.
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
     * Create a random cryptographic key (K_cred) associated with the given slotID. K_cred is
     * protected with two layers of encryption. First, it is encrypted with the protector-key. The
     * output blob of the first encryption is then encrypted with a per-slot key.
     * Return the GW blob which protects K_cred with these two layers of encryption
     *
     * @param slotID - index of the array of slots representing the GateWeaver storage, that
     *                 corresponds to the GW blob output by this method call.
     *
     * @param scheduleID - the ratelimiting schedule to be used during verification. If a
     *                     verification attempt fails, GW records the number of consecutive failure
     *                     attempts and the wait time enforced until the next verification attempt
     *                     is allowed, associated with the slotID. The wait time until next
     *                     verification attempt is retrieved from the ratelimiting schedule given
     *                     the number of failed verification attempts.
     */
    Arc create(in int slotID, in int scheduleID, in byte[] protector_key);

    Arc verify(in slotID, in Arc gateweaverBlob, in byte[] protector_key);
}
