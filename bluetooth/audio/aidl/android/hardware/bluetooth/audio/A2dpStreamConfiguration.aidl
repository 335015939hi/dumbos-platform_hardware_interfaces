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

@VintfStability
parcelable A2dpStreamConfiguration {
    /**
     * Peer MTU (16 bits)
     */
    int peerMtu;

    /**
     * Content protection by SCMS-T
     */
    boolean isScmstEnabled;

    /**
     * Codec Selection and configuration, as defined by the `Codec Specific
     * Information Elements` [AVDTP - 8.21.5], or `Vendor Specific Value`
     * [A2DP - 4.7.2] when CodecId format is set to `VENDOR`.
     */
    CodecId codecId;
    byte[] configuration;
}
