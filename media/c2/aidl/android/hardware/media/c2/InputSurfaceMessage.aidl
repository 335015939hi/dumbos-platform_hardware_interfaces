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

package android.hardware.media.c2;

/**
 * Message from IInputSurface to the client.
 *
 * IInputSurface can pass the current buffer stream processing information to
 * the client via FMQ. the each message's structure is as below.
 */
@VintfStability
@FixedSize
parcelable InputSurfaceMessage {
    /** const definition for message types */
    const int MESSAGE_ON_FRAME_AVAILABLE = 0;

    const int MESSAGE_ON_FRAME_ACQUIRED = 1;

    const int MESSAGE_ON_FRAME_RELEASED = 2;

    const int MESSAGE_ON_EOS = 3;

    /**
     * Message Type
     */
    int messageType;

    /**
     * Frame Sequeunce Id
     */
    int frameSeqId;

    /**
     * Number of frames for the message.
     *
     * The sequence id for frames will be
     * from frameSeqId to frameSeqId + (count - 1).
     */
    int count;

    /**
     * The error reason for MESSAGE_ON_FRAME_RELEASED.
     *
     * c2_status_t
     * C2_OK : when frames are queued successfully.
     * C2_CANCELED : dropped (encoder cannot catch up the feeding speed.)
     */
    int error;

    /**
     * Timestamp of the event w.r.t IInputSurface.
     */
    long timestamp;
}
