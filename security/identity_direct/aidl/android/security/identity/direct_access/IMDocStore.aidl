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
package android.security.identity.direct_access;

import android.hardware.identity.Certificate;
import android.security.identity.direct_access.IMDocCredential;

@VintfStability
interface IMDocStore {
    /**
     * Success. This is never returned but included for completeness and for use by code
     * using these statuses for internal use.
     */
    const int STATUS_OK = 0;
    /**
     * The operation failed.  This is used as a generic catch-all for errors that don't belong
     * in other categories, including memory/resource allocation failures and I/O errors.
     */
    const int STATUS_FAILED = 1;
    /**
     * When attempting to to retrieve the credentail from an empty slot.
     */
    const int STATUS_NO_SUCH_CREDENTIAL = 2;
    /**
     * When attempting to create a credential in a slot where a credential has already been created.
     */
    const int STATUS_CREDENTIAL_ALREADY_EXISTS = 3;
    /**
     * The credential data is greater than the maximum supported credentail data size.
     * Implementations should support least 32 KiB.
     */
    const int STATUS_CREDENTIAL_DATA_TOO_BIG = 4;
    /**
     * The provided presentation package is not valid.
     */
    const int STATUS_PRESENTATION_PACKAGE_INVALID = 5;
    /**
     * A request to retrieve a presentation package where no presentation package is selected.
     */
    const int STATUS_NO_PRESENTATION_PACKAGE_SELECTED = 6;
    /**
     * A request to use a production key to simulate presentation.
     */
    const int STATUS_NOT_TEST_CREDENTIAL = 7;
    /**
     * This is used when a malformed `deviceRequestCbor` is detected.
     */
    const int STATUS_INVALID_CBOR = 8;

    /**
     * Gets the number of credentials that can be stored in Secure Hardware.
     *
     * @return number of slots. This is at least 1 but can be larger.
     */
    int getNumberOfCredentialSlots();

    /**
     * Creates a new credential in the given slot. Must support `challenge`
     * being at least XX bytes long.
     *
     * @param credentialSlot the slot in which new credential is created.
     *
     * @param testCredential indicates if this is a test store.
     *    If testCredential is set to `true` the TA shall use all zeroes for the
     *    HBK for encrypting the data in MDocPresentationPackage. Additionally,
     *    the credential shall never be presented to an MDOC reader over NFC.
     *
     * @param challenge contains a byte string from the Issuing Authority.
     *
     * @return a credentialKey attestation for the newly created credential.
     *    Fails with STATUS_CREDENTIAL_ALREADY_EXISTS if the given
     *    slot is already in use.
     */
    Certificate[] createMDocCredential(
            in int credentialSlot, in boolean testCredential, in byte[] challenge);

    /**
     * Looks up a previously created credential.
     *
     * @param credentialSlot the slot in which to look for an MDocCredential.
     *
     * @return an MDocCredentail . see IMDocCredential.aidl
     *    Fails with STATUS_NO_SUCH_CREDENTIAL if no MDocCredentail is present
     *    in the provided slot.
     */
    IMDocCredential lookupMDocCredential(in int credentialSlot);

    /**
     * Deletes a previously created credential.
     *
     * @param credentialSlot the slot in which to delete the MDocCredential
     *
     * Fails with STATUS_NO_SUCH_CREDENTIAL if no MDocCredentail is present
     *    in the provided slot.
     */
    void deleteMDocCredential(in int credentialSlot);

    /**
     * Gets the maximum size of `credentialData` which can be used in
     * presentation packages. This must be at least 32 KiB.
     *
     * @return the maximum credentailData size that can be used in
     *    prsentation packages.
     */
    long getMaximumCredentialDataSize();
}
