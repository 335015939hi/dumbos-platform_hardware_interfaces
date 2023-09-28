/*
 * Copyright 2021 The Android Open Source Project
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
import android.hardware.bluetooth.audio.CodecParameters;

/**
 * A2DP Service Configuration
 */
@VintfStability
parcelable A2dpConfiguration {
    /**
     * Remote Stream Endpoint Identifier
     */
    int remoteSeid;

    /**
     * Codec Selection and configuration, in a generic way and as defined
     * by the A2DP's `Codec Specific Information Elements`,
     * or `Vendor Specific Value` when CodecId format is set to `VENDOR`.
     */
    CodecId id;
    CodecParameters parameters;
    byte[] configuration;
}
