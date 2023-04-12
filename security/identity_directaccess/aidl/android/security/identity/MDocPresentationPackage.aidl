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

/**
 * A presentation package is defined as the unit containing all data needed to
 * present a credential, including the docType, the signing key, the credential
 * PII, the MSO and access control directives such as reader authentication.
 *
 * This structure is uniquely identified by the X.509 certificate for the
 * signing key.
 *
 * The private part of the signing key, the PII of the credential, and the
 * MSO is also in this data structure but encrypted so only the Secure
 * Hardware can access it.
 */
@VintfStability
parcelable MDocPresentationPackage {
    /**
     * The certificate representing the signing key.
     */
    Certificate signingKeyCertificate;

    /**
     * Encrypted data which is encrypted using AES-GCM-128 encryption
     *
     *   AES-GCM-ENC(HBK, R, SigningKeyAndCredentialData, CredentialKey_pub)
     *
     * where HBK is a unique hardware-bound key that has never existed outside
     * of the Secure Hardware, R is a nonce, CredentialKey_pub is the public
     * part of CredentialKey in uncompressed form, and SigningKeysAndCredential
     * contains the bytes of CBOR described by the following CDDL:
     *
     *   SigningKeyAndCredentialData = [
     *     bstr,          ; the private signing key in uncompressed form
     *     bstr / nil,    ; bytes of CBOR for CredentialData, if available
     *   }
     *
     * If testCredential is set to true for the credential slot that the
     * presentation package belongs to, the HBK used shall be all zeroes.
     *
     * TODO: specify R must be random and generated in secure area
     */
    byte[] encryptedData;
}
