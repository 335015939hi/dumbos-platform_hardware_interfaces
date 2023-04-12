/*
 * Copyright 2023 The Android Open Source Project
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
package android.security.identity;

import android.hardware.identity.Certificate;
import android.security.identity.IMDocCredential;

@VintfStability
interface IMDocStore {
    const int STATUS_FAILED = 1;
    const int STATUS_NO_SUCH_CREDENTIAL = 2;
    const int STATUS_CREDENTIAL_ALREADY_EXISTS = 3;
    const int STATUS_CREDENTIAL_DATA_TOO_BIG = 4;
    const int STATUS_PRESENTATION_PACKAGE_INVALID = 5;
    const int STATUS_NO_PRESENTATION_PACKAGE_SELECTED = 6;
    const int STATUS_NOT_TEST_CREDENTIAL = 7;
    const int STATUS_INVALID_CBOR = 8;

    /**
     * Gets the number of credentials that can be stored in Secure Hardware.
     *
     * @return
     *    Returns the number of slots. This is at least 1 but can be larger.
     */
    int getNumberOfCredentialSlots();

    /**
     * Creates a new credential in the given slot. Must support `challenge`
     * being at least XX bytes long.
     *
     * @param credentialSlot The slot in which new credential is created.
     *
     * @param testCredential indicates if this is a test store.
     *    If testCredential is set to `true` the TA shall use all zeroes for the
     *    HBK for encrypting the data in MDocPresentationPackage. Additionally,
     *    the credential shall never be presented to an MDOC reader over NFC.
     *
     * @param challenge contains a byte string from the Issuing Authority.
     *
     * @return a credentialKey attestation for the newly created credential
     *    on success. Fails with STATUS_CREDENTIAL_ALREADY_EXISTS if the given
     *    slot is already in use.
     */
    @nullable List<Certificate> createMDocCredential(
            in int credentialSlot, in boolean testCredential, in byte[] challenge);

    /**
     * Looks up a previously created credential.
     *
     * @param credentialSlot
     *
     * @return
     */
    @nullable IMDocCredential lookupMDocCredential(in int credentialSlot);

    /**
     * Deletes a previously created credential.
     *
     * @param credentialSlot
     */
    void deleteMDocCredential(in int credentialSlot);

    /**
     * Gets the maximum size of `credentialData` which can be used in
     * presentation packages. This must be at least 32 KiB.
     *
     * @return
     */
    long getMaximumCredentialDataSize();
}
