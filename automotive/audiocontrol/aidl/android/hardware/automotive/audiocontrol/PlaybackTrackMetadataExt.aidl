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

package android.hardware.automotive.audiocontrol;

import android.hardware.automotive.audiocontrol.PlaybackTrackMetadata;
import android.media.audio.common.AudioUsage;
import android.media.audio.common.AudioContentType;

/**
 * The extension of the qualifier of a given audio use cases holding the given
 * {@link android.hardware.automotive.audiocontrol.PlaybackTrackMetadata}
 * for a single audio zone.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable PlaybackTrackMetadataExt {
    /**
     * The identifier for the audio zone the audio stream following the associated AudioAttributes
     * belongs to.
     */
    int zoneId;

    PlaybackTrackMetadata playbackTrackMetadata;
}
