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
 * All parameters defined in union AcousticEchoCanceler must be gettable and settable. The
 * capabilities defined in AcousticEchoCanceler.Capability can only acquired with
 * IEffect.getDescriptor() and not settable.
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
     * Must be inside the range defined in capability.
     */
    int echoDelayUs;
    /**
     * If AEC mobile mode enabled.
     * Can only be false if AEC implementation indicate not support mobile mode by set
     * range of parameter Id mobileMode to false in capability.
     */
    boolean mobileMode;
}
