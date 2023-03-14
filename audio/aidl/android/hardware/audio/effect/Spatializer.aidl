/*
 * Copyright (C) 2023 The Android Open Source Project
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
import android.media.audio.common.AudioChannelLayout;

/**
 * Spatializer specific definitions.
 *
 * All parameter settings must be inside the range of Capability.Range.spatializer definition if the
 * definition for the corresponding parameter tag exist. See more detals about Range in Range.aidl.
 */
@VintfStability
union Spatializer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        VendorExtension vendorExtensionTag;
        Spatializer.Tag commonTag;
    }

    /**
     * Vendor Equalizer implementation definition for additional parameters.
     */
    VendorExtension vendor;

    /**
     * The level of spatialization.
     * Align with SpatializationLevel.aidl.
     */
    @VintfStability
    @Backing(type="byte")
    enum Level {

        /**
         * Spatialization is disabled.
         */
        NONE = 0,
        /**
         * The spatializer accepts audio with positional multichannel masks (e.g 5.1).
         */
        MULTICHANNEL = 1,
        /**
         * The spatializer accepts audio made of a channel bed of positional multichannels
         * (e.g 5.1) and audio objects positioned independently via meta data.
         */
        MCHAN_BED_PLUS_OBJECTS = 2,
    }
    Level level;

    /**
     * The head tracking mode.
     * Align with SpatializerHeadTrackingMode.aidl.
     */
    @VintfStability
    @Backing(type="byte")
    enum HeadTrackingMode {

        /**
         * Head tracking is active in a mode not listed below (forward compatibility).
         */
        OTHER = 0,

        /**
         * Head tracking is disabled
         */
        DISABLED = 1,

        /**
         * Head tracking is performed relative to the real work environment.
         */
        RELATIVE_WORLD = 2,

        /**
         * Head tracking is performed relative to the device's screen.
         */
        RELATIVE_SCREEN = 3,
    }
    HeadTrackingMode headTrackingMode;

    /**
     * List of supported input channel masks.
     */
    AudioChannelLayout[] supportedChannelLayout;

    /**
     * The spatialization mode.
     * Align with SpatializationMode.aidl.
     */
    @Backing(type="byte")
    enum Mode {
        /**
         * The spatializer supports binaural mode (over headphones type devices).
         */
        BINAURAL = 0,
        /**
         * The spatializer supports transaural mode (over speaker type devices).
         */
        TRANSAURAL = 1,
    }
    Mode mode;

    /**
     * Vector of 6 floats representing the head to stage pose:
     * First three are a translation vector and the last three are a rotation vector.
     */
    const int HEAD_TO_STRING_VEC_SIZE = 6;
    float[HEAD_TO_STRING_VEC_SIZE] headToStage;

    /**
     * Display orientation  as reported by DisplayManager (float value in radian).
     *
     * Only 4 values 0, PI/2, PI, 3PI/2 will be sent as of Android 14 (U).
     * Due to precision, compare with an epsilon range, suggest rounding to the nearest integer
     * degree for practical use.
     *
     * Notes:
     *    1) A device may have more than one display.
     *    2) A display may be locked which prevents the application from rotating.
     */
    float displayOrientation;

    /**
     * Foldable device hinge angle as a float value in rad.
     */
    float hingeAngle;

    /**
     * The fold state as reported by DeviceStateManager for a foldable device.
     * This is an integer value of either 0 (open) or 1 (folded).
     */
    boolean foldState;
}
