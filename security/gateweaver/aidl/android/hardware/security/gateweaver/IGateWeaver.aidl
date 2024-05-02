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
import android.hardware.security.gateweaver.ReEnrollResponse;
import android.hardware.security.gateweaver.TimestampPackage;

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
 *                the GW blob associated with the slot. Wait time is computed as: now + wait time
 *                defined by the ratelimiting schedule for a given failure count. A ratelimiting
 *                schedule is an array indexed by the failure count.
 *
 * AuthGraph enabled GateWeaver provides two artifacts:
 * 1) a unique cryptographic key for encrypting LSKF-gated user data,
 * 2) a proof of user authentication, and makes them available to any component which need them.
 *
 * GW implementation should declare whether it needs to be informed of the secure time, in which
 * case, the caller should provide the timestamp obtained from a secure clock service to the
 * `verify` method call. GW needs to be informed of the identity (i.e. persistent signing key or the
 * DICE chain) of the secure clock service in an implementation dependent way. A shared key should
 * be setup in every boot between the GW and the secure clock service via the IAuthGraphKeyExchange
 * protocol.
 *
 * TODO: mention that GW should have access to a monotonically increasing clock (should it be
 * ticking even when the device is powered off?) to enforce ratelimiting.
 * @hide
 */
@VintfStability
/*@SensitiveData - TODO: temporarily disable this to avoid the compiler error as in b/298159959*/
interface IGateWeaver {
    /**
     * Create a random cryptographic key (K_cred). K_cred is protected with two layers of
     * encryption. First, it is encrypted with the protector key that is provided as an input. The
     * output blob of the first encryption is then encrypted with the per-slot encryption key
     * associated with the given slot id. Return the GW blob which protects K_cred with these two
     * layers of encryption.
     *
     * Steps:
     * 1) Create K_cred - a 256-bit cryptographic key for AES-GCM encryption
     * 2) Create an arc encrypting K_cred with the `protectorKey`
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
     *                     TODO: finalize how the schedule is defined
     *
     * @param protectorKey - a 256-bit cryptographic key for AES-GCM encryption
     *                       TODO: define how to derive this from LSKF
     *
     * @return the GW blob
     * TODO: define the errors that can be returned by create.
     */
    Arc create(in int slotID, in int scheduleID, in byte[] protectorKey);

    /**
     * Decrypt K_cred from the `gateweaverBlob` and return an `Arc` encrypting K_cred with the
     * per-boot key of the GW. This returned arc is intended to be used by the
     * AuthGraphKeyManagement API implementation of the GW.
     *
     * K_cred can be recovered only if:
     * 1) the per-slot encryption key associated with the slot id of the GW blob is not deleted
     * 2) the same protector-secret key that was provided when creating K_cred is provided as an
     * input to “verify”.
     * GW applies SE or TEE based rate limiting on failed verification attempts, based on the
     * ratelimiting schedule that was specified during creation of the GW blob.
     *
     * Steps:
     * 1) Retrive slot_id from the protected headers of the `gateWeaverBlob`
     * 2) Retrieve the encryption_key associated with the slot id
     * 3) Decrypt the `gateWeaverBlob` with the encryption_key
     * 4) Retrieve the UID and the ratelimiting_schedule id from the protected headers of the
     *    `gateWeaverBlob`
     * 5) If the failure_count associated with the slot_id > 0, check if now < wait_time associated
     *    with the slot_id. If so, return error: THROTTLED along with time till next allowed
     *    attempt = wait_time - now. Otherwise, proceed.
     * 6) Decrypt the output of step #3 with the `protectorKey` to recover K_cred
     * 7) If step #6 succeeds;
     *        i) reset the failure_count and wait_time associated with the slot id to zero
     *       ii) create an arc encrypting K_cred with the per-boot key of GW with the following
     *           protected headers:
     *               a) UID - obtained from the GW blob in step #4
     *               b) KeyType = CredentialKey
     * 8) If step #6 fails;
     *        i) set failure_count = failure_count + 1
     *       ii) if the failure_count > length of the rate limiting schedule, set failure_count =
     *           length of the rate limiting schedule
     *      iii) set wait_time = now + ratelimiting_schedule[failure_count - 1]
     *       iv) return error: THROTTLED along with time till next allowed attempt = wait_time
     *
     * @param gateWeaverBlob - a GW blob returned by a previous call to `create`
     *
     * @param protectorKey - the same protector key that was provided to `create` when the GW blob
     *                       was created
     *
     * @param challenge - an optional challenge (from the consumer of the proof of user
     *                    authentication produced by the GW) or 0
     *
     * @param timestamp - an optional arc providing the authenticated time from a secure clock
     *                    service implementing the ISecureClockService.aidl
     *
     * @return arc from step 7.ii on success or the error response on errors as described above
     * // TODO: look into existing re-enroll
     */
    Arc verify(in Arc gateWeaverBlob, in byte[] protectorKey, in long challenge,
            in @nullable TimestampPackage timestamp);

    /**
     * Reset the encryption_key
     */
    void invalidate(in int slotID);

    ReEnrollResponse reEnroll();
}
