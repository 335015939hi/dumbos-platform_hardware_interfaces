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
 * This interface is used to indicate completion of asynchronous operations.
 * See the state machine described in StreamDescriptor for details.
 */
@VintfStability
interface IStreamCallback {
    /**
     * Indicate that the stream is ready for the next data exchange.
     */
    oneway void onTransferReady();
    /**
     * Provide an update on draining.
     *
     * The HAL module must call this method periodically while draining audio
     * output buffers in response to the DRAIN command.
     *
     * @param remainingMs remaining playback time, 0 if draining has finished.
     */
    oneway void onDrainUpdate(int remainingMs);
    /**
     * Indicate that an error has occurred during the last I/O operation.
     */
    oneway void onError();
}
