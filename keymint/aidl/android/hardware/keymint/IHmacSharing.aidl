/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.keymint;

import android.hardware.keymint.HmacSharingParameters;


/* This interface defines the sharing of HMAC keys with another IHmacSharing implementation of
 * other security levels.
 */
@VintfStability interface IHmacSharing {
    /**
     * Start the creation of an HMAC key, shared with another IKeyMintDevice implementation.  Any
     * device with a StrongBox IKeyMintDevice has two IKeyMintDevice instances, because there
     * must be a TEE Keymint as well.  The HMAC key used to MAC and verify authentication tokens
     * (HardwareAuthToken, VerificationToken and ConfirmationToken all use this HMAC key) must be
     * shared between TEE and StrongBox so they can each validate tokens produced by the other.
     * This method is the first step in the process for agreeing on a shared key.  It is called by
     * Android during startup.  The system calls it on each of the HAL instances and collects the
     * results in preparation for the second step.
     *
     * @return error ErrorCode::OK on success, ErrorCode::UNIMPLEMENTED if HMAC agreement is not
     *         implemented (note that all 4.0::IKeyMintDevice HALS must implement HMAC agreement,
     *         regardless of whether or not the HAL will be used on a device with StrongBox), or
     *         ErrorCode::UNKNOWN_ERROR if the parameters cannot be returned.
     *
     * @return params The HmacSharingParameters to use.  As specified in the HmacSharingParameters
     *         documentation in types.hal, the seed must contain the same value in every invocation
     *         of the method on a given device, and the nonce must return the same value for every
     *         invocation during a boot session.
     */
    HmacSharingParameters getHmacSharingParameters();

    /**
     * Complete the creation of an HMAC key, shared with another IKeyMintDevice implementation.
     * Any device with a StrongBox IKeyMintDevice has two IKeyMintDevice instances, because
     * there must be a TEE IKeyMintDevice as well.  The HMAC key used to MAC and verify
     * authentication tokens must be shared between TEE and StrongBox so they can each validate
     * tokens produced by the other.  This method is the second and final step in the process for
     * agreeing on a shared key.  It is called by Android during startup.  The system calls it on
     * each of the HAL instances, and sends to it all of the HmacSharingParameters returned by all
     * HALs.
     *
     * To ensure consistent ordering of the HmacSharingParameters, the caller must sort the
     * parameters lexicographically.  See the support/keymint_utils.cpp for an operator< that
     * defines the appropriate ordering.
     *
     * This method computes the shared 32-byte HMAC ``H'' as follows (all IKeyMintDevice instances
     * perform the same computation to arrive at the same result):
     *
     *     H = CKDF(key = K,
     *              context = P1 || P2 || ... || Pn,
     *              label = "KeymintSharedMac")
     *
     * where:
     *
     *     ``CKDF'' is the standard AES-CMAC KDF from NIST SP 800-108 in counter mode (see Section
     *           5.1 of the referenced publication).  ``key'', ``context'', and ``label'' are
     *           defined in the standard.  The counter is prefixed and length L appended, as shown
     *           in the construction on page 12 of the standard.  The label string is UTF-8 encoded.
     *
     *     ``K'' is a pre-established shared secret, set up during factory reset.  The mechanism for
     *           establishing this shared secret is implementation-defined, but see below for a
     *           recommended approach, which assumes that the TEE IKeyMintDevice does not have
     *           storage available to it, but the StrongBox IKeyMintDevice does.
     *
     *           CRITICAL SECURITY REQUIREMENT: All keys created by a IKeyMintDevice instance must
     *           be cryptographically bound to the value of K, such that establishing a new K
     *           permanently destroys them.
     *
     *     ``||'' represents concatenation.
     *
     *     ``Pi'' is the i'th HmacSharingParameters value in the params vector.  Note that at
     *           present only two IKeyMintDevice implementations are supported, but this mechanism
     *           extends without modification to any number of implementations.  Encoding of an
     *           HmacSharingParameters is the concatenation of its two fields, i.e. seed || nonce.
     *
     * Note that the label "KeymintSharedMac" is the 18-byte UTF-8 encoding of the string.
     *
     * Process for establishing K:
     *
     *     Any method of securely establishing K that ensures that an attacker cannot obtain or
     *     derive its value is acceptable.  What follows is a recommended approach, to be executed
     *     during each factory reset.  It relies on use of the factory-installed attestation keys to
     *     mitigate person-in-the-middle attacks.  This protocol requires that one of the instances
     *     have secure persistent storage.  This model was chosen because StrongBox has secure
     *     persistent storage (by definition), but the TEE may not.  The instance without storage is
     *     assumed to be able to derive a unique hardware-bound key (HBK) which is used only for
     *     this purpose, and is not derivable outside the secure environment.
     *
     *     In what follows, T is the IKeyMintDevice instance without storage, S is the
     *     IKeyMintDevice instance with storage:
     *
     *     1. T generates an ephemeral EC P-256 key pair K1.
     *     2. T sends K1_pub to S, signed with T's attestation key.
     *     3. S validates the signature on K1_pub.
     *     4. S generates an ephemeral EC P-256 key pair K2.
     *     5. S sends {K1_pub, K2_pub}, to T, signed with S's attestation key.
     *     6. T validates the signature on {K1_pub, K2_pub}.
     *     7. T uses {K1_priv, K2_pub} with ECDH to compute session secret Q.
     *     8. T generates a random seed S.
     *     9. T computes K = KDF(HBK, S), where KDF is some secure key derivation function.
     *     10. T sends M = AES-GCM-ENCRYPT(Q, {S || K}) to S.
     *     10. S uses {K2_priv, K1_pub} with ECDH to compute session secret Q.
     *     11. S computes S || K = AES-GCM-DECRYPT(Q, M) and stores S and K.
     *
     *     When S receives the getHmacSharingParameters call, it returns the stored S as the seed
     *     and a nonce.  When T receives the same call, it returns an empty seed and a nonce.  When
     *     T receives the computeSharedHmac call, it uses the seed provided by S to compute K.  S,
     *     of course, has K stored.
     *
     * @param params The HmacSharingParameters data returned by all IKeyMintDevice instances when
     *        getHmacSharingParameters was called.
     *
     * @return error ErrorCode::OK in the event that there is no error.  ErrorCode::INVALID_ARGUMENT
     *         if one of the provided parameters is not the value returned by the prior call to
     *         getHmacParameters().
     *
     * @return sharingCheck A 32-byte value used to verify that all IKeyMintDevice instances have
     *         computed the same shared HMAC key.  The sharingCheck value is computed as follows:
     *
     *             sharingCheck = HMAC(H, "Keymint HMAC Verification")
     *
     *         The string is UTF-8 encoded, 27 bytes in length.  If the returned values of all
     *         IKeyMintDevice instances don't match, clients must assume that HMAC agreement
     *         failed.
     */
    byte[] computeSharedHmac(in HmacSharingParameters[] params);
}
