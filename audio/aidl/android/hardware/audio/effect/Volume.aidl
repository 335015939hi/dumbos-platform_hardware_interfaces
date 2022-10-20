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
 * Volume specific definitions.
 *
 * All parameters defined in union Volume must be gettable and settable. The capabilities defined in
 * Volume.Capability can only acquired with IEffect.getDescriptor() and not settable.
 */
@VintfStability
union Volume {
    /**
     * Vendor Volume implementation definition for additional parameters.
     */
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;

    /**
     * Capability MUST be supported by Volume implementation.
     */
    @VintfStability
    parcelable Capability {
        /**
         * Volume capability extension, vendor can use this extension in case existing capability
         * definition not enough.
         */
        ParcelableHolder extension;

        /**
         * Volume strength supported.
         */
        int maxLevel;
    }

    /**
     * Current level.
     */
    int level;
    /**
     * Mute volume if true, when volume set to mute, the current level still saved and take effect
     * when unmute.
     */
    boolean mute;
    /**
     * Stereo position enabled if true.
     */
    boolean stereoPositionEnabled;
    /**
     * Only enabled when stereoPositionEnabled is true.
     */
    int stereoPosition;
}
