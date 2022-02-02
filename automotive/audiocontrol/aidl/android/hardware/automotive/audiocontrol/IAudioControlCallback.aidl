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

import android.hardware.automotive.audiocontrol.AudioFocusChange;
import android.hardware.automotive.audiocontrol.AudioGainConfigInfo;
import android.hardware.automotive.audiocontrol.PlaybackTrackMetadata;

/**
 * Interface definition for a callback to be invoked
 *      - when the audio focus of the system is updated
 *      - when the gain(s) of the device port(s) is(are) updated
 */
@VintfStability
oneway interface IAudioControlCallback {

    /**
     * Used to indicate that the audio output stream associated with playbackMetaData has released
     * the focus.
     *
     * @param playbackMetaData The output stream metadata associated with the focus request
     * @param zoneId The audio zone associated to the focus request
     */
    void abandonAudioFocus(in PlaybackTrackMetadata playbackMetaData, in int zoneId);

    /**
     * Used to indicate that the audio output stream associated with playbackMetaData has taken
     * the focus.
     *
     * @param playbackMetaData The output stream metadata associated with the focus request
     * @param zoneId The audio zone associated to the focus request
     * @param focusGain The focus type requested.
     *                  This must be one of the
     *                  {@link android.hardware.automotive.audiocontrol.AudioFocusChange} constants
     */
    void requestAudioFocus(in PlaybackTrackMetadata playbackMetaData, in int zoneId,
          in AudioFocusChange focusGain);


    /**
     * Used to indicated the one or more audio device port gains have changed unexpectidely.
     *
     * @param reasons One or more reasons that triggered the given gains changed.
     *                This must be one or more of the
     *                {@link android.hardware.automotive.audiocontrol.Reasons} constants.
     *
     * @param gains List of gains affected by the change.
     */
    void onAudioDevicePortGainsChanged(in int[] reasons, in AudioGainConfigInfo[] gains);
}
