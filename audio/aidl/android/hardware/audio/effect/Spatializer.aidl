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
 * Spatializer specific definitions.
 *
 * All parameters defined in union Spatializer must be gettable and settable. The capabilities
 * defined in Spatializer.Capability can only acquired with IEffect.getDescriptor() and not
 * settable.
 */
@VintfStability
union Spatializer {
    /**
     * Effect parameter tag to identify the parameters for getParameter().
     */
    @VintfStability
    union Id {
        int vendorExtensionTag;
        Spatializer.Tag tag;
    }

    /**
     * Vendor Spatializer implementation definition for additional parameters.
     */
    @VintfStability
    parcelable VendorExtension {
        ParcelableHolder extension;
    }
    VendorExtension vendor;
}
