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

import android.hardware.audio.common.SourceMetadata;

/**
 * This interface provides means for sending audio data to output devices.
 */
@VintfStability
interface IStreamOut {
    /**
     * Close the stream.
     *
     * Releases any resources allocated for this stream on the HAL module side.
     * This includes the fast message queues and shared memories returned via
     * the StreamDescriptor. Thus, the stream can not be operated anymore after
     * it has been closed. The client needs to release the audio data I/O
     * objects after the call to this method returns.
     *
     * Methods of this interface throw EX_ILLEGAL_STATE for a closed stream.
     *
     * @throws EX_ILLEGAL_STATE If the stream has already been closed.
     */
    void close();

    /**
     * Pause audio playback at the observer's end of the pipeline.
     *
     * This method is intended for use in cases when the stream receives large
     * chunks of audio data per burst (for example, because the latency is high,
     * or data is compressed), thus putting audio data exchange on hold from the
     * client side would be noted by the observer only after a non-negligible
     * delay. Calling this method should result in ceasing of any playback
     * at the observer's end as soon as possible.
     *
     * Audio data which has not yet been consumed retains in buffers. While the
     * stream remains in the paused state, audio hardware may still be using
     * power. The client may consider calling the 'standby' method after a
     * timeout to prevent excess power usage.
     *
     * It is allowed for streams not to support pause and resume operations
     * (always in pair) if it is known that the output latency is low.
     * Support for pause and resume is required for offloaded streams.
     *
     * @throws EX_ILLEGAL_STATE When called on a closed stream, a stream
     *                          which is in the "standby" state, or has already
     *                          been paused.
     * @throws EX_UNSUPPORTED_OPERATION If both pause and resume are not
     *                                  supported for this stream.
     */
    void pause();

    /**
     * Resume audio playback at the observer's end of the pipeline.
     *
     * Notifies the stream to resume playback following a pause. This also
     * cancels the standby state.
     *
     * It is allowed for streams not to support pause and resume operations
     * (always in pair) if it is known that the output latency is low.
     * Support for pause and resume is required for offloaded streams.
     *
     * @throws EX_ILLEGAL_STATE When called on a closed stream, or a stream
     *                          which is not paused.
     * @throws EX_UNSUPPORTED_OPERATION If both pause and resume are not
     *                                  supported for this stream.
     */
    void resume();

    /**
     * Suggest putting the stream into a "standby" state to save power.
     *
     * Hints the HAL module that the client is not going to perform audio I/O
     * for some time, thus the HAL module can put any connected hardware into
     * standby mode to save power. This requires that the stream is currently
     * put "on hold" (that is, has just been opened, or was put on hold via
     * the associated StreamDescriptor).
     *
     * It's left on the discretion of the HAL implementation to assess all the
     * necessary conditions that could prevent hardware from being suspended,
     * and ignore the call in this case. Inability to suspend the hardware is
     * not considered as an error.
     *
     * @throws EX_ILLEGAL_STATE If not applicable, this includes the following
     *                          conditions:
     *                           - the stream is closed;
     *                           - the stream is already in the standby state;
     *                           - the audio exchange was not put on hold.
     */
    void standby();

    /**
     * Update stream metadata.
     *
     * Updates the metadata initially provided at the stream creation.
     *
     * @param sourceMetadata Updated metadata.
     * @throws EX_ILLEGAL_STATE If the stream is closed.
     */
    void updateMetadata(in SourceMetadata sourceMetadata);
}
