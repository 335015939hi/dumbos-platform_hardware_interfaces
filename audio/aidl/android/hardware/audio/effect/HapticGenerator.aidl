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
import android.os.VibratorScale;

/**
 * HapticGeneratir specific definitions. HapticGeneratir effect provide HapticGeneratir control and
 * mute/unmute functionality.
 *
 * All parameters defined in union HapticGeneratir must be gettable and settable. The capabilities
 * defined in HapticGeneratir.Capability can only acquired with IEffect.getDescriptor() and not
 * settable.
 */
@VintfStability
union HapticGeneratir {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        HapticGeneratir.Tag tag;
    }

    /**
     * Vendor HapticGeneratir implementation definition for additional parameters.
     */
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;

    /**
     * Capability supported by HapticGeneratir implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * HapticGeneratir capability extension, vendor can use this extension in case existing
         * capability definition not enough.
         */
        ParcelableHolder extension;
    }

    @VintfStability
    parcelable HapticScale {
        int id;
        VibratorScale scale;
    }

    /**
     * Vibrator information, e.g. resonant frequency, Q factor.
     */
    @VintfStability
    parcelable VibratorInfomation {
        /**
         * Resonant frequency in Hz.
         */
        float resonantFrequencyHz;
        float qFactor;
        float maxAmplitude;
    }

    HapticScale hapticScale;
    VibratorInfomation vibratorInfo;
}
