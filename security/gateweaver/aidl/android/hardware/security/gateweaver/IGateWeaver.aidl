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
import android.hardware.security.gateweaver.GateWeaverConfig;
import android.hardware.security.gateweaver.ReEnrollResponse;
import android.hardware.security.gateweaver.TimestampPackage;
import android.hardware.security.gateweaver.VerifyResponse;
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
 *                defined by the ratelimiting schedule for a given failure count.
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
     * Retrieves the config information for this implementation of GateWeaver.
     * The config is static i.e. every invocation returns the same information.
     *
     * @return config information for this implementation of GateWeaver
     */
    GateWeaverConfig getConfig();
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
     *     i) UID - a randomly generated unique identifier of 32 bytes
     *    ii) slot_id - the slot id given as input
     *   iii) source_id - an opaquue value that allows GW to (later) identify the security version
     *                    that this GW blob was created in (e.g. hash of the GW's DICE policy)
     *
     * @param slotID - (unused) index in the array of slots representing the GateWeaver storage, to
     *                 be associated with the GW blob output by this method call.
     *
     * @param protectorKey - a 256-bit cryptographic key for AES-GCM encryption
     *
     * @return the GW blob
     */
    Arc create(in int slotID, in byte[] protectorKey);

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
     * 1) Retrieve slot_id from the protected headers of the `gateWeaverBlob`
     * 2) Retrieve the encryption_key associated with the slot id
     * 3) Decrypt the `gateWeaverBlob` with the encryption_key, if the decryption fails, return
     *    error: INVALID_BLOB, otherwise, proceed.
     * 4) If the failure_count > maximum number of allowed retry attempts, return error:
     *    MAXIMUM_ATTEMPTS_REACHED, else proceed.
     * 5) If the failure_count > 0, check if now < wait_time. If so, return error: THROTTLED
     *    along with time till next allowed attempt = wait_time - now. Otherwise, proceed.
     * 6) Set failure_count = failure_count + 1, set wait_time = now + wait time defined by the
     *    ratelimiting schedule for the current failure count
     * 7) Decrypt the output of step #3 with the `protectorKey` to recover K_cred
     * 8) If step #6 succeeds;
     *        i) reset the failure_count and wait_time to zero
     *       ii) create an arc encrypting K_cred with the per-boot key of GW with the following
     *           protected headers:
     *               a) source_id = 'Identity' of GW
     *               b) UID = the UID obtained from the protected headers of the GW blob
     *               c) KeyType = CredKey
     *               d) challenge = the challenge from the input
     *               e) Timestamp = the timestamp obtained from the timestamp arc
     *      iii) check whether the `source_id` matches the current security version of the GW. If
     *           not, return the arc created in step ii, along with the status code
     *           RE_ENROLL_REQUIRED. Otherwise, return the arc with the status code OK.
     * 9) If step #6 fails, return error: THROTTLED along with time till next allowed attempt =
     *    wait_time
     *
     * @param gateWeaverBlob - a GW blob returned by a previous call to `create`
     *
     * @param protectorKey - the same protector key that was provided to `create` when the GW blob
     *                       was created
     *
     * @param challenge - an optional challenge (from the consumer of the proof of user
     *                    authentication produced by the GW) or 0
     *
     * @param timestamp - an optional arc providing the authenticated time from a secure time
     *                    service implementing the ISecureClockService.aidl
     *
     * @return VerifyResponse return arc from step 8.ii on success or the corresponding
     *         error response on failures as described above.
     */
    VerifyResponse verify(in Arc gateWeaverBlob, in byte[] protectorKey, in long challenge,
            in @nullable TimestampPackage timestamp);

    /**
     * Reset the slot in the array of slots representing the GW storage, that is identified by the
     * given slot id.
     */
    void invalidate(in int slotID);

    /**
     * Re-enroll is has multiple usages. E.g.: 1) Changing the user's LSKF (lock screen knowledge
     * factor - i.e. PIN, pattern, password). 2) Adding a token based protector for the LSKF
     * protected secrets 3) Upgrading GW blobs upon a security upgrade of the GateWeaver.
     *
     * First, internally call `verify` on the provided GW blob, with the `oldProtectorKey` and the
     * timestamp. This ensures that ratelimiting is applied during re-enroll as well. If it is
     * successful, move on to creating the new GW blob. Otherwise, return the error returned from
     * `verify`.
     *
     * Follow the steps of `create` to create a new GW blob encrypted by the new protector key and
     * the encryption key associated with the given slot id. However, instead of a
     * randomly generated uniique identifier, use the UID included in the protected headers of the
     * GW blob provided as input. Return the outputs of internal calls to `create` and `verify`.
     *
     * It is the responsibility of the caller to perform the subsequent operations to complete the
     * task of re-enroll, based on the use case.
     * In all three use cases, the caller should make sure that the downstream secrets protected
     * from the old K_cred are migrated to be protected by the new K_cred.
     * In use case 1, the old GW blob should be deleted. Note that the deletion of the GW blob
     * should be performed in a separate step to ensure crash robustness.
     * In use case 3, the old GW blob as well as the downstream secrets protected by the old K_cred
     * should be deleted. The caller should maintain a reference count for all the downstream parent
     * secrets and make sure that any parent secret is deleted only when its reference count is zero
     * (i.e. all child secrets are migrated to the new parent secrets).
     *
     * @param gateWeaverBlob - the GW blob protected by the oldProtectorKey
     *
     * @param oldProtectorKey - the protector key that is used to provide the inner encryption for
     *                          the gateWeaverBlob
     *
     * @param timestamp - an optional arc providing the authenticated time from a secure time
     *                    service implementing the ISecureClockService.aidl
     *
     * @param slotID - (unused) index in the array of slots representing the GateWeaver storage, to
     *                 be associated with the new GW blob.
     *
     * @param newProtectorKey - a 256-bit cryptographic key for AES-GCM encryption
     *
     * @return the new GW blob and an arc encrypting the old K_cred with the per-boot key of the GW
     *         upon successful re-enroll or the error response from the internal call to `verify`
     */
    ReEnrollResponse reEnroll(in Arc gateWeaverBlob, in byte[] oldProtectorKey,
            in @nullable TimestampPackage timestamp, in int slotID,
            in byte[] newProtectorKey);

    /* TODO: add the AuthGraph KE and KM APIs */
}
