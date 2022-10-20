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

/**
 * Bass boost is an audio effect to boost or amplify low frequencies of the sound. It is comparable
 * to a simple equalizer but limited to one band amplification in the low frequency range.
 *
 * All parameters defined in union BassBoost must be gettable and settable. The capabilities defined
 * in BassBoost.Capability can only acquired with IEffect.getDescriptor() and not settable.
 */
@VintfStability
union BassBoost {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        BassBoost.Tag tag;
    }

    /**
     * Vendor BassBoost implementation definition for additional parameters.
     */
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;

    /**
     * Capability MUST be supported by BassBoost implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * BassBoost capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        ParcelableHolder extension;
        /**
         * Is strength parameter supported.
         */
        boolean strengthSupported;
        /**
         * The max strength supported by effect implementation. The minimal strength support will
         * always be 0 which designates the mildest effect.
         */
        int maxStrengthSupported;
    }

    /**
     * The strength of the bass boost effect.
     *
     * If the implementation does not support per mille accuracy for setting the strength, it is
     * allowed to round the given strength to the nearest supported value. You can use the {@link
     * #IEffect.getParameter(Parameter.Specific.BassBoost.strength)} method to query the (possibly
     * rounded) value that was actually set.
     *
     * The valid range for strength is [0, Capabicity.maxStrengthSupported].
     */
    int strength;
}
