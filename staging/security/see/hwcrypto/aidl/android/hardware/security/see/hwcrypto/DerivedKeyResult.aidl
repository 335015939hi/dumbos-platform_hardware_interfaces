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
package android.hardware.security.see.hwcrypto;

import android.hardware.security.see.hwcrypto.HwCryptoKeyMaterial;

/*
 * parcelable containing the result of calling
 * <code>IHwCryptoDeviceKeyAccess::hwkeyDeriveVersioned</code>.
 */
parcelable DerivedKeyResult {
    /*
     * Union containing either an opaque or a clear key.
     */
    HwCryptoKeyMaterial keyMaterial;

    /*
     * Most current dice policy if <code>IHwCryptoDeviceKeyAccess::hwkeyDeriveVersioned</code> was
     * called with an old policy or a NULL one. It will be NULL otherwise. This policy is opaque
     * from this service perspective (it will be send to an Authentication Manager Service to be
     * verified) but it will follow the structure defined on the DicePolicy.cddl, with the caveat
     * that it could be encrypted if the client do not have enough permissions to see the device
     * dice policy information. The reason to return the most current dice policy if an old one
     * was provided is for the client to detect this case and rotate its keys if so desired. Old,
     * valid policies remain usable, but care needs to be taken to not continue to use a potentially
     * compromised key.
     */
    @nullable byte[] dicePolicyForKeyVersion;
}
