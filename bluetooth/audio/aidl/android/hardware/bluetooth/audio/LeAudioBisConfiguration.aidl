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

/**
 * LE Audio BIS configuration
 */
@VintfStability
parcelable LeAudioBisConfiguration {
    /**
     * Codec ID
     */
    CodecId codecId;

    /**
     * Codec configuration for BIS or group of BISes. This will also be used to
     * verify the requirements on the known LTV types.
     */
    CodecSpecificConfigurationLtv[] codecConfiguration;

    /**
     * Vendor specific codec configuration.
     * This will not be parsed by stack but will be used as the codec specific
     * configuration. If this is populated, only `vendorCodecConfiguration` will
     * be used, otherwise `codecConfiguration` will be used. Vendor shall put
     * any parameters defined by the Assigned Numbers it decided to reuse for
     * this particular vendor codec configuration if it uses at least one vendor
     * specific parameter.
     */
    byte[] vendorCodecConfiguration;

    /**
     * Metadata for a particular BIS or group of BISes. This is optional
     */
    @nullable MetadataLtv[] metadata;
}
