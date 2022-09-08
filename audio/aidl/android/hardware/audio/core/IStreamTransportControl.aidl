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

/**
 * This is a complementary interface to IStream*, intended to provide high-level
 * control functionality over the audio stream transport. It is used when audio
 * data is exchanged in large chunks or directly through DMA buffers (MMap mode).
 */
@VintfStability
interface IStreamTransportControl {
    /**
     * Stop capturing or playing audio.
     *
     * Audio data which has not yet been consumed retains in buffers. While the
     * stream remains in the paused state, audio hardware may still be using
     * power. The client may consider calling the 'IStream*.standby' method
     * after a timeout to prevent excess power usage.
     *
     * @throws EX_ILLEGAL_STATE When called on a closed stream, a stream
     *                          which is in the "standby" state, or has already
     *                          been paused.
     */
    void pause();

    /**
     * Resume capturing or playing audio.
     *
     * Notifies the stream to resume playback following a pause.
     *
     * @throws EX_ILLEGAL_STATE When called on a closed stream, or a stream
     *                          which is not paused.
     */
    void resume();
}
