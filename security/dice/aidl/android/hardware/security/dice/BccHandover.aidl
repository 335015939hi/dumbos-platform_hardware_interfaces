/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.security.dice;

/**
 * Represents one set of DICE artifacts in BCC handover format.
 * 
 * The handover format is a CBOR map with three fields:
 *
 * BccHandover = {
 *   1 : bstr .size 32,     ; CDI_Attest
 *   2 : bstr .size 32,     ; CDI_Seal
 *   3 : Bcc,               ; Certificate chain
 * }
 *
 * The format of Bcc can be found here:
 * https://cs.android.com/android/platform/superproject/+/master:hardware/interfaces/security/keymint/aidl/android/hardware/security/keymint/ProtectedData.aidl
 *
 * @hide
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
parcelable BccHandover {
    /**
     * CDI_Seal.
     */
    byte[] data;
}
