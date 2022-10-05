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

import android.hardware.audio.core.AudioMode;

/**
 * An instance of ITelephony manages settings which are specific to voice calls
 * and SMS messaging functionality. This interface is optional to implement and
 * provide by the vendor. It needs to be provided only if the device actually
 * supports telephony.
 */
@VintfStability
interface ITelephony {
    /**
     * Return the list of supported audio modes.
     *
     * The first 4 AudioModes: NORMAL, RINGTONE, IN_CALL, IN_COMMUNICATION must
     * be supported by all implementations.
     *
     * This method is only called once, during the audio system initialization,
     * and must return the same result all the time.
     *
     * @return The list of supported audio modes.
     */
    AudioMode[] getSupportedAudioModes();

    /**
     * Notify the HAL on the change of the current audio mode.
     *
     * The current audio mode is always controlled by the client. The HAL must
     * accept all modes returned by 'getSupportedAudioModes' and reject the rest.
     *
     * Since this is a blocking (synchronous) call, the HAL must return from it
     * only after switching itself to the specified mode.
     *
     * @param mode The current mode.
     * @throws EX_UNSUPPORTED_OPERATION If the HAL does not support the specified
     *                                  mode.
     */
    void updateAudioMode(AudioMode mode);
}
