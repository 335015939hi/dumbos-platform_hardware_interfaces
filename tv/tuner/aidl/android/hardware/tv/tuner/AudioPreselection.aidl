/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.tuner;

import android.hardware.tv.tuner.AudioPreselectionMessageDescriptor;
import android.hardware.tv.tuner.AudioPreselectionRenderingIndicationType;

/**
 * Audio preselection metadata according to ETSI EN 300 468 V1.17.1.
 * @hide
 */
@VintfStability
parcelable AudioPreselection {
    int preselectionId;

    /**
     * Not specified in ETSI EN 300 486 V1.17.1, but can be used by some codecs (e.g. Dolby AC-4)
     * to ensure contininuity of personalized experience during program transitions.
     */
    int programId;

    /**
     * Collection of audio preselection message descriptors.
     */
    AudioPreselectionMessageDescriptor[] messageDescriptors;

    /**
     * ISO 639-2 3-character code.
     */
    String language;

    /**
     * A hint for a preferred reproduction channel layout.
     */
    AudioPreselectionRenderingIndicationType renderingIndication;

    /**
     * Audio preselection contains audio description for the visually impaired.
     */
    boolean hasAudioDescription;

    /**
     * Audio preselection contains spoken subtitles.
     */
    boolean hasSpokenSubtitles;

    /**
     * Audio preselection provides support for dialogue enhancement.
     */
    boolean hasDialogueEnhancement;
}
