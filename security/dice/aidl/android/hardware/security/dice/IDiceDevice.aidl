/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.security.dice;

import android.hardware.security.dice.Artifacts;
import android.hardware.security.dice.Certificate;
import android.hardware.security.dice.InputValues;
import android.hardware.security.dice.Sealed;
import android.hardware.security.dice.Signature;

/**
 * Dice device definition.
 *
 * == Features ==
 *
 * The dice device provides access to the component's CDI_SEAL and CDI_ATTEST secrets as well
 * as to its attestation certificate chain. The "component" is the Android instance running this
 * this HAL service and the secrets and attestation chain must include all boot stage components,
 * the kernel, and system.image.
 *
 * Implementations provide the following operations:
 * * sign - Signing a payload with a key derived from CDI_ATTEST.
 * * getAttestationChain - Retrieve the component's attestation certificate chain.
 * * derive - Retrieve the component's DICE artifacts.
 * @hide
 */
@SensitiveData
@VintfStability
interface IDiceDevice {
    /**
     * Uses the component's, or a child's given by `inputValues`, sealing secret to encrypt the
     * plaintext using AES256-GCM. Implementations use a random salt and the kdfInfo provided by
     * the caller to derive an encryption key. (TODO specify KDF) The sealing secret is guaranteed
     * to be stable for between factory resets but must be rotated on factory reset.
     * The return value is a structure, that contains the cipher text and all
     * metadata (Salt, IV, AEAD tag) required to unseal the data except the kdfInfo. The plaintext
     * argument is unstructured but limited to 512 bytes. It is recommended to use this mechanism
     * to seal a session key when sealing larger chunks of data.
     */
    Sealed seal(in InputValues[] InputValues, in byte[] kdfInfo, in byte[] plaintext);

    /**
     * Takes a `Sealed` structure as returned by `seal()` and decrypts the data using the caller's
     * sealing secret. The caller must also provide the same `inputValues` and `kdfInfo` used
     * during sealing.
     *
     * @return plaintext as passed to `seal()` when `sealed` was created.
     *
     * ## Error as service specific exception:
     *     ResponseCode::VERIFICATION_FAILED if the integrity of sealed could not be established.
     */
    byte[] unseal(in InputValues[] inputValues, in byte[] kdfInfo, in Sealed sealed);

    /**
     * Uses the a key derived from the component's, or a child's given by `inputValues`,
     * attestation secret to sign the payload using RFC 8032 PureEd25519 and returns the
     * signature. The payload is limited to 1024 bytes.
     */
    Signature sign(in InputValues[] id, in byte[] payload);

    /**
     * Returns the attestation chain of the component if `inputValues` is empty or the chain
     * to the given child of the component identified by the `inputValues` vector.
     *
     * ## Error as service specific exception:
     *     ResponseCode::PERMISSION_DENIED if the caller is not sufficiently privileged.
     */
    Certificate[] getAttestationChain(in InputValues[] inputValues);

    /**
     * This function allows a client to become a resident node. Called with empty `inputValues`
     * vectors, an implementation returns the component's DICE secrets. If the `inputValues` vector
     * is given the appropriate derivations are performed starting from the component's level.
     *
     * ## Error as service specific exception:
     *     ResponseCode::PERMISSION_DENIED if the implementation does not allow resident nodes
     *     at the client's level.
     */
    Artifacts derive(in InputValues[] inputValues);

    /**
     * This demotes the implementation.
     * When called, the implementation performs appropriate derivation steps using
     * `inputValues`, traversing the vector in ascending order. Then it replaces its
     * stored DICE artifacts with the newly derived ones.
     *
     * IMPORTANT: When the function returns, all remnants of the previous DICE artifacts must
     * have been purged from memory.
     *
     * This operation is not reversible until the next reboot. Further demotion is always
     * possible.
     *
     * ## Error as service specific exception:
     *     ResponseCode::DEMOTION_FAILED if the implementation failed to demote itself
     *     or was unable to purge previous DICE artifacts from memory.
     */
    void demote(in InputValues[] inputValues);
}
