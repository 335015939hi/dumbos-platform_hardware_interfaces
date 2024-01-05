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

package android.hardware.audio.effect;

import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * This interface provides a mechanism for the audio effect instance to notify the audio framework
 * about changes in data FMQ (Fast Message Queue).
 */
@VintfStability
oneway interface IEffectCallback {
    /**
     * Notifies the audio framework of changes in the input data FMQ (Fast Message Queue) from
     * the effect instance.
     *
     * The effect instance may choose to reallocate the input data message queue under specific
     * conditions. For example, when the input format changes, requiring an update to the
     * data message queue allocated during the open time. In such cases, the effect instance can
     * create a new data message queue and pass it to the audio framework.
     *
     * In the callback implementation, the audio framework must deprecate the input data queue
     * obtained at open time and transition to using the new data queue for subsequent operations.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the dataMQ parameter is determined to be invalid.
     */
    void onInputDataFmqChanged(in MQDescriptor<float, SynchronizedReadWrite> dataMQ);

    /**
     * Notifies the audio framework of changes in the output data FMQ (Fast Message Queue) from
     * the effect instance.
     *
     * The effect instance may decide to reallocate the output data message queue under specific
     * conditions. For example, when the output format changes, requiring an update to the
     * data message queue allocated during the open time. In such cases, the effect instance can
     * create a new data message queue and pass it to the audio framework.
     *
     * In the callback implementation, the audio framework must deprecate the output data queue
     * obtained at open time and transition to using the new data queue for subsequent operations.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the dataMQ parameter is determined to be invalid.
     */
    void onOutputDataFmqChanged(in MQDescriptor<float, SynchronizedReadWrite> dataMQ);
}
