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

import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.LtvData;

/**
 * LE Audio ASE configuration
 */
@VintfStability
parcelable LeAudioAseConfiguration {
    /**
     *  Target latency used in Configure Codec command
     */
    int targetLatency;

    /**
     *  Target PHY used in Configure Codec command
     */
    int targetPhy;

    /**
     * Codec ID
     */
    CodecId codecId;

    /**
     * Codec configuration for ASE. This shall contain all the LTVs but
     * allocation. Audio Channel Allocation will be added by the
     * Bluetooth stack.
     */
    LtvData[] codecConfiguration;

    /**
     * Metadata, packed as LTV - used to enable ASE. This is optional
     */
    @nullable LtvData[] metadata;
}
