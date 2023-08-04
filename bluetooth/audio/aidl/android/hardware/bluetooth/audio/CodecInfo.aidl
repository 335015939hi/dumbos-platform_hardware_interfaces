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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.ChannelMode;
import android.hardware.bluetooth.audio.CodecId;

/**
 * General information about a Codec
 */
@VintfStability
parcelable CodecInfo {
    /**
     * Codec identifier and human readable name
     */
    CodecId id;
    String name;

    /**
     * The capabilities as defined by the A2DP's `Codec Specific Information
     * Elements`, or `Vendor Specific Value` when CodecId format is set to `VENDOR`.
     */
    ParcelableHolder a2dpCapabilities;

    /**
     * PCM characteristics:
     * - Mono, Dual-Mono or Stereo
     * - Supported sampling frequencies, in Hz.
     * - Fixed point resolution, basically 16, 24 or 32 bits by samples.
     *   The value 32 should be used for floating point representation.
     */
    ChannelMode[] channelMode;
    int[] samplingFrequencyHz;
    int[] bitdepth;

    /**
     * Coding characteristics:
     * - Lossless capable
     * - Is "Low-Latency" or able to run in a Low-Latency mode.
     */
    boolean lowLatency;
    boolean lossless;
}
