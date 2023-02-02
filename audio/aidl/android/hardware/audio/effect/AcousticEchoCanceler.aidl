/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.audio.effect;

import android.hardware.audio.effect.VendorExtension;

/**
 * Acoustic Echo Canceler (AEC) is an audio pre-processor which removes the contribution of the
 * signal received from the remote party from the captured audio signal.
 *
 * All parameter settings must be inside the range of Capability.Range.acousticEchoCanceler
 * definition if the definition for the corresponding parameter tag exist. See more detals about
 * Range in Range.aidl.
 */
@VintfStability
union AcousticEchoCanceler {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        AcousticEchoCanceler.Tag commonTag;
    }

    /**
     * Vendor AEC implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * The AEC echo delay in microseconds.
     */
    int echoDelayUs;
    /**
     * If AEC mobile mode enabled.
     * If effect implementation support enable/disable mobileMode in runtime, it can either not
     * setting anything for the range, or report the mobileMode range as [false, true] in
     * Range.AcousticEchoCancelerRange. If effect implementation doesn't support mobileMode, it must
     * report the mobileMode range as [false, false]. If effect implementation can only support
     * mobileMode, it must report the mobileMode range as [true, true]. If effect implementation set
     * the range as [true, false], it indicates that mobildeMode setParameter is not supported, and
     * client can only use getParameter to check if its enabled or not.
     */
    boolean mobileMode;
}
