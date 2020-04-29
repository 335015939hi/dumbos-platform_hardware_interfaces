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


/**
 * HmacSharingParameters holds the data used in the process of establishing a shared HMAC key
 * between multiple Keymint instances.  Sharing parameters are returned in this struct by
 * getHmacSharingParameters() and send to computeSharedHmac().  See the named methods in
 * IKeyMintDevice for details of usage.
 */
@VintfStability
parcelable HmacSharingParameters {
    /**
     * Either empty or contains a persistent value that is associated with the pre-shared HMAC
     * agreement key.  It is either empty or 32 bytes in length.
     */
    byte[] seed;

    /**
     * A 32-byte value which is guaranteed to be different each time
     * getHmacSharingParameters() is called.  Probabilistic uniqueness (i.e. random) is acceptable,
     * though a stronger uniqueness guarantee (e.g. counter) is recommended where possible.
     */
    byte[] nonce;
}
