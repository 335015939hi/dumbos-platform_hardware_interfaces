/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.hardware.keymaster;

import android.hardware.keymaster.SecurityLevel;


/**
 * KeymasterHardwareInfo is the hardware information returned by calling Keymaster getHardwareInfo()
 */

@VintfStability
parcelable KeymasterHardwareInfo {
    /* securityLevel is the security level of the KeymasterDevice implementation accessed
     * through this aidl package.  */
    SecurityLevel securityLevel;

    /* keymasterName is the name of the IKeymasterDevice implementation.  */
    @utf8InCpp String keymasterName;

    /* keymasterAuthorName is the name of the author of the IKeymasterDevice implementation
     *         (organization name, not individual).
     */
    @utf8InCpp String keymasterAuthorName;
}
