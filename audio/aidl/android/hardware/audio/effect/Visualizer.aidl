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
 * Visualizer specific definitions.
 *
 * All parameters defined in union Visualizer must be gettable and settable. The capabilities
 * defined in Visualizer.Capability can only acquired with IEffect.getDescriptor() and not
 * settable.
 */
@VintfStability
union Visualizer {
    /**
     * Vendor Visualizer implementation definition for additional parameters.
     */
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;

    /**
     * Capability MUST be supported by Visualizer implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * Visualizer capability extension, vendor can use this extension in case existing
         * capability definition not enough.
         */
        ParcelableHolder extension;

        /**
         * Max latency supported.
         */
        int maxLatency;
        /**
         *  Capture size range.
         */
        CaptureSizeRange captureSizeRange;
    }

    /**
     * Supported capture size range in samples.
     */
    @VintfStability
    parcelable CaptureSizeRange {
        int min;
        int max;
    }

    /**
     * Supported capture size range in samples.
     */
    @VintfStability
    enum ScalingMode {
        /**
         * Defines a capture mode where amplification is applied based on the content of the
         * captured data. This is the default Visualizer mode, and is suitable for music
         * visualization.
         */
        NORMALZED = 0,
        /**
         * Defines a capture mode where the playback volume will affect (scale) the range of the
         * captured data. A low playback volume will lead to low sample and fft values, and
         * vice-versa.
         */
        AS_PLAYED,
    }

    /**
     * Supported capture size range in samples.
     */
    @VintfStability
    enum MeasurementMode {
        /**
         * No measurements are performed.
         */
        NONE = 0,
        /**
         * Defines a measurement mode which computes the peak and RMS value in mB below the "full
         * scale", where 0mB is normally the maximum sample value (but see the note below). Minimum
         * value depends on the resolution of audio samples used by the audio framework. The value
         * of -9600mB is the minimum value for 16-bit audio systems and -14400mB or below for "high
         * resolution" systems. Values for peak and RMS can be retrieved with {@link
         * #getMeasurementPeakRms(MeasurementPeakRms)}.
         */
        PEAK_RMS,
    }

    /**
     * Current capture size. The capture size must be a power of 2 in the range
     * Capability.captureSizeRange.
     */
    int captureSize;
    /**
     * Visualizer latency.
     */
    int latency;
    /**
     * Visualizer capture mode
     */
    ScalingMode scalingMode;
    /**
     * Visualizer measurement mode.
     */
    MeasurementMode measurementMode;
}
