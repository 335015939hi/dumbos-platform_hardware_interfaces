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
 * Automatic Gain Control (AGC) is an audio pre-processor which automatically normalizes the output
 * of the captured signal by boosting or lowering input from the microphone to match a preset level
 * so that the output signal level is virtually constant. AGC can be used by applications where the
 * input signal dynamic range is not important but where a constant strong capture level is desired.
 * WebRTC only supports RMS level estimator and saturation margin is no longer configurable.
 *
 * All parameters defined in union AutomaticGainControlV2 must be gettable and settable. The
 * capabilities defined in AutomaticGainControlV2.Capability can only acquired with
 * IEffect.getDescriptor() and not settable.
 */
@VintfStability
union AutomaticGainControlV2 {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        AutomaticGainControlV2.Tag commonTag;
    }

    /**
     * Vendor AutomaticGainControlV2 implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * Capability supported by AutomaticGainControlV2 implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * AutomaticGainControlV2 capability extension, vendor can use this extension in case
         * existing capability definition not enough.
         */
        ParcelableHolder extension;
        /**
         * Max fixed digital gain supported by AGC implementation in millibel.
         */
        int maxFixedDigitalGainMb;
    }

    /**
     * The AGC fixed digital gain in millibel.
     * Must never be negative, and not larger than maxFixedDigitalGainMb in capability.
     */
    int fixedDigitalGainMb;
}
