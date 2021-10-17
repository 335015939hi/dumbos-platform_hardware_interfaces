/*
 * Copyright (C) 2021 The Android Open Source Project
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

import android.hardware.bluetooth.audio.AudioContentType;
import android.hardware.bluetooth.audio.AudioUsage;

@VintfStability
parcelable PlaybackTrackMetadata {
    AudioUsage usage;
    AudioContentType contentType;
    /**
     * Positive linear gain applied to the track samples. 0 being muted and 1 is no attenuation,
     * 2 means double amplification...
     * Must not be negative.
     */
    float gain;
}
