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

import android.hardware.audio.effect.Descriptor;
import android.media.audio.common.AudioSource;
import android.media.audio.common.AudioStreamType;
import android.media.audio.common.AudioUuid;

/**
 * List of effects which should be used for the pre-process or post-process of certain audio stream
 * type.
 */
@VintfStability
parcelable Process {
    // Type of the process.
    @VintfStability
    union Type {
        // Common parameter tag.
        AudioStreamType streamType = AudioStreamType.INVALID;
        // Vendor defined parameter tag.
        AudioSource source;
    }

    Type type;
    // List of effect identities for this process.
    Descriptor.Identity[] ids;
}
