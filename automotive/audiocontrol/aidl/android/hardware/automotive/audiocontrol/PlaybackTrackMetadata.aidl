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

import android.media.audio.common.AudioUsage;
import android.media.audio.common.AudioContentType;

@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable PlaybackTrackMetadata {
    /**
     * Usage expresses why you are playing a sound and what this sound is used for.
     * This enum corresponds to AudioAttributes.USAGE_* constants in the SDK.
     */
    AudioUsage usage = AudioUsage.INVALID;

    /**
     * Content type specifies "what" is playing. The content type expresses the
     * general category of the content: speech, music, movie audio, etc.
     * This enum corresponds to AudioAttributes.CONTENT_TYPE_* constants in the SDK.
     */
    AudioContentType contentType = AudioContentType.UNKNOWN;

    /**
     * Tags from AudioTrack audio attributes. Tag is an additional use case
     * qualifier complementing AudioUsage and AudioContentType. Tags are set by
     * vendor specific applications and must be prefixed by "VX_". Vendor must
     * namespace their tag names to avoid conflicts, for example:
     * "VX_GOOGLE_VR". At least 3 characters are required for the vendor
     * namespace.
     */
    @utf8InCpp String[] tags;
}
