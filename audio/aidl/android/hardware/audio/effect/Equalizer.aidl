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

import android.media.audio.common.AudioProfile;

/**
 * Equalizer specific definitions.
 */
@VintfStability
union Equalizer {
    // Vendor Equalizer implementation definition for additional parameters.
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;

    /**
     * Capability MUST be supported by Equalizer implementationx.
     */
    @VintfStability
    parcelable Capability {
        /**
         * Equalizer capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        ParcelableHolder extension;

        // Bands frequency ranges supported.
        Band[] bands;

        // Presets name and index.
        Preset[] presets;
    }

    /**
     * Parameters MUST be supported by Equalizer implementation.
     */
    /**
     * Band information.
     */
    @VintfStability
    parcelable Band {
        int index; // Index of the band.
        int level; // current level setting.
        int minFreq; // minimal frequency.
        int maxFreq; // maximum frequency.
        int centre; // centre frequency.
    }

    /**
     * Factory presets supported.
     */
    @VintfStability
    parcelable Preset {
        // Index of the preset.
        int index;
        // Preset name, used to identify presets but no intended to display on UI directly.
        @utf8InCpp String name;
    }

    // List of bands with frequence range and current level for get and set parameter.
    Band[] bands;
    // presets with name and index for getParameter.
    Preset[] presets;
    // current preset for get and setParameter.
    int preset;
}
