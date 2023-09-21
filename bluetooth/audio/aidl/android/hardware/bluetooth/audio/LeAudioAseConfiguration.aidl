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
import android.hardware.bluetooth.audio.CodecSpecificConfigurationLtv;
import android.hardware.bluetooth.audio.MetadataLtv;
import android.hardware.bluetooth.audio.Phy;

/**
 * LE Audio ASE configuration
 */
@VintfStability
parcelable LeAudioAseConfiguration {
    @VintfStability
    @Backing(type="byte")
    enum TargetLatency {
        LOWER = 0x01,
        BALANCED_LATENCY_RELIABILITY = 0x02,
        HIGHER_RELIABILITY = 0x03,
    }

    /**
     *  Target latency used in Configure Codec command
     */
    TargetLatency targetLatency;

    /**
     *  Target PHY used in Configure Codec command
     */
    Phy targetPhy;

    /**
     * Codec ID
     */
    CodecId codecId;

    /**
     * Codec configuration for ASE represented in the LTV types defined by
     * Bluetooht SIG. Regardless of vendor specific configuration being used or
     * not, this shall contain Bluetooth LTV types describing the common stream
     * parameters, at least sample frequency. In addition it should match
     * aseConfiguration provided in LeAudioConfigurationRequirement as this will
     * also be used to verify the requirements on the known LTV types.
     *
     * Note, this configuration shall not contain audio channel allocation.
     * Audio Channel Allocation will be added by the Bluetooth stack unless
     * DONT_USE_AUDIO_ALLOCATIONS flag is set in the configuration returned by
     * the offloader or if vendorCodecConfiguration is used.
     */
    CodecSpecificConfigurationLtv[] codecConfiguration;

    /**
     * Vendor specific codec configuration for ASE.
     *
     * This will not be parsed by the stack but will be used as the codec
     * specific configuration in codec configure control point operation for an
     * ASE. If this is populated, only `vendorCodecConfiguration` will be used
     * for ASE configuration, otherwise `codecConfiguration` will be used. The
     * BT stack will not merge it with the codecConfiguration for any purpose.
     * Vendor shall put any parameters defined by the Assigned Numbers if needed
     * for this particular vendor codec configuration.
     */
    byte[] vendorCodecConfiguration;

    /**
     * Metadata, packed as LTV - used to enable ASE. This is optional
     */
    @nullable MetadataLtv[] metadata;
}
