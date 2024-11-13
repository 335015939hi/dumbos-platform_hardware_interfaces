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

package android.hardware.security.secretkeeper;

/**
 * Contents of a pubkey.
 * @hide
 */
@VintfStability
parcelable PublicKey {
    /**
     * CBOR-encoded COSE_Key, as a PubKeyEd25519 / PubKeyECDSA256 / PubKeyECDSA384
     * as defined in generateCertificateRequestV2.cddl"
     */
    byte[] keyMaterial;
}
