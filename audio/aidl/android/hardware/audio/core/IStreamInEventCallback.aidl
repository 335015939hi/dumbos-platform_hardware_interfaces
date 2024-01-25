/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
 * This interface provides means for asynchronous notification to the client
 * by an input stream.
 */
@VintfStability
oneway interface IStreamInEventCallback {
    /**
     * Called with the array of volumes per channel when an external volume request change occurs,
     * the volume (attenuation) can't be applied in hardware, and are forwarded to input stream
     * clients to apply.
     *
     * Attenuation requests from external sources can be received when the source is not able to
     * apply volume, for example because the stream is encoded.
     * NOTE: these requests are different from gain applied in hardware via IStreamIn setHwGain,
     * but are attenuation requests received by the input stream source from the HAL hardware
     * source.
     *
     * The valid range for attenuation is [0.0f, 1.0f], where 1.0f corresponds
     * to unity gain, 0.0f corresponds to full mute (see REMOTE_VOLUME_* constants).
     *
     * @param volumes An array of requested attenuation values to be applied for each input channel.
     */
    const int REMOTE_VOLUME_MIN = 0;
    const int REMOTE_VOLUME_MAX = 1;
    /**
     */
    void onVolumeChanged(in float[] volumes);
}
