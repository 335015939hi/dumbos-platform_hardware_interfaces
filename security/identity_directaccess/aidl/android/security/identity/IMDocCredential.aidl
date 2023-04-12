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
import android.security.identity.MDocPresentationPackage;

@VintfStability
interface IMDocCredential {
    /**
     * Generates a new MDOC presentation package.
     * The returned structure contains a X.509 certificate for the signing
     * key with a validity period from and |validityPeriodMillis| milliseconds
     * in the future. This is signed by CredentialKey and should be sent to the
     * issuer for certification. Once the issuer has certified this, the
     * application should call presentationPackageSetData().
     * Implementations must support creating an unlimited number of
     * presentation packages.
     *
     * @param validityPeriodMillis
     *
     * @return
     */
    MDocPresentationPackage presentationPackageGenerate(in long validityPeriodMillis);

    /**
     * Sets the credential data for a presentation package. The passed in
     * data is the same format as specified in the Framework API's
     * MDocCredential.provision() method.
     * This returns the presentation package but with the PII and MSO replaced
     * by the data passed in.
     *
     * @param presentationPackage
     *
     * @param credentialData
     *
     * @return
     */
    MDocPresentationPackage presentationPackageSetData(
            in MDocPresentationPackage presentationPackage, in byte[] credentialData);

    /**
     * Gets the currently selected presentation package.
     * Logically this would return a MDocPresentationPackage but for
     * optimization, only the Certificate part of a MDocSigningKey is returned.
     *
     * @return
     */
    Certificate currentPresentationPackageGet();

    /**
     * Sets the current presentation package. This resets the use count to 0.
     *
     * @param presentationPackage
     */
    void currentPresentationPackageSet(in MDocPresentationPackage presentationPackage);

    /**
     * Clears the current presentation package.
     */
    void currentPresentationPackageClear();

    /**
     * Gets the number of times the current presentation package has been used.
     *
     * @return
     */
    int currentPresentationPackageGetNumUses();

    /**
     * Simulates an MDOC presentation.
     *
     * This only works for test credentials and is only used for testing.
     * `deviceRequestCbor` contains the bytes of DeviceRequest CBOR according to
     * ISO/IEC 18013-5:2021 and the return value shall contain the bytes of
     * DeviceResponse CBOR.
     *
     * The following SessionTranscript shall be used:
     *
     *   SessionTranscript = [
     *       nil,           ; DeviceEngagementBytes is not set.
     *       nil,           ; EReaderKeyBytes is not set.
     *       nil,           ; Handover is nil.
     *   ]
     *
     * Fails with STATUS_NOT_TEST_CREDENTIAL if this is not a test credential.
     *
     * Fails with STATUS_INVALID_CBOR if `deviceRequestCbor` is malformed.
     *
     * @param deviceRequestCbor
     *
     * @return
     */
    byte[] simulatePresentation(in byte[] deviceRequestCbor);
}
