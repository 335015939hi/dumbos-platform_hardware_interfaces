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

package android.hardware.audio.core;

// !!! Do not submit as is !!!
// !!! Move this file to android.media.audio.common and remove
// !!! frameworks/av/media/libaudioclient/aidl/android/media/AudioDualMonoMode.aidl

/* Dual Mono handling is used when a stereo audio stream contains separate
 * audio content on the left and right channels. Such information about the
 * content of the stream may be found, for example, in ITU T-REC-J.94-201610
 * A.6.2.3 Component descriptor.
 */
@VintfStability
@Backing(type="int")
enum DualMonoMode {
    // Needs to be in sync with DUAL_MONO_MODE* constants in
    // frameworks/base/media/java/android/media/AudioTrack.java
    /**
     * Disable any Dual Mono presentation effect.
     */
    OFF = 0,
    /**
     * This mode indicates that a stereo stream should be presented
     * with the left and right audio channels blended together
     * and delivered to both channels.
     *
     * Behavior for non-stereo streams is implementation defined.
     * A suggested guideline is that the left-right stereo symmetric
     * channels are pairwise blended, the other channels such as center
     * are left alone.
     */
    LR = 1,
    /**
     * This mode indicates that a stereo stream should be presented
     * with the left audio channel replicated into the right audio channel.
     *
     * Behavior for non-stereo streams is implementation defined.
     * A suggested guideline is that all channels with left-right
     * stereo symmetry will have the left channel position replicated
     * into the right channel position. The center channels (with no
     * left/right symmetry) or unbalanced channels are left alone.
     */
    LL = 2,
    /**
     * This mode indicates that a stereo stream should be presented
     * with the right audio channel replicated into the left audio channel.
     *
     * Behavior for non-stereo streams is implementation defined.
     * A suggested guideline is that all channels with left-right
     * stereo symmetry will have the right channel position replicated
     * into the left channel position. The center channels (with no
     * left/right symmetry) or unbalanced channels are left alone.
     */
    RR = 3,
}
