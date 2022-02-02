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

import android.hardware.audio.common.PlaybackTrackMetadata;

/**
 * Extension of {@link android.media.AudioAttributes} like.
 *
 * <p>The extension of the qualifier of a given audio use cases holding the given
 * {@link android.hardware.audio.common.PlaybackTrackMetadata}
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

    /**
     * Metadata of a playback track for an output stream.
     * This is highly closed to {@link android.media.AudioAttributes}.
     * It allows to identify the audio stream rendered / requesting / abandonning the focus.
     *
     * AudioControl 1.0 was limited to identification through {@code AttributeUsage} listed as
     * {@code audioUsage} in audio_policy_configuration.xsd.
     *
     * Any new OEM needs would not be possible without extension.
     *
     * Relying on {@link android.hardware.automotive.audiocontrol.PlaybackTrackMetadata} allows
     * to use a combination of {@code AttributeUsage}, {@code AttributeContentType} and
     * {@code AttributeTags} to identify the use case / routing thanks to
     * {@link android.media.audiopolicy.AudioProductStrategy}.
     * The belonging to a strategy is deduced by an AOSP logic (in sync at native and java layer).
     *
     * IMPORTANT NOTE ON TAGS:
     * To limit the possibilies and prevent from confusion, we expect the String to follow
     * a given formalism that will be enforced.
     *
     * 1 / By convention, tags shall be a "key=value" pair.
     * Vendor must namespace their tag's key (for example com.google.strategy=VR) to avoid conflicts.
     * vendor specific applications and must be prefixed by "VX_". Vendor must
     *
     * 2 / Tags reported here shall be the same as the tags used to define a given
     * {@link android.media.audiopolicy.AudioProductStrategy} and so in
     * audio_policy_engine_configuration.xml file.
     */
    PlaybackTrackMetadata playbackTrackMetadata;
}
