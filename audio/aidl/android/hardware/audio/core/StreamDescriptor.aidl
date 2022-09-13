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

import android.hardware.audio.core.MmapBufferDescriptor;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * Stream descriptor contains fast message queues and buffers used for sending
 * and receiving audio data. The descriptor complements IStream* interfaces by
 * providing communication channels that serve as an alternative to Binder
 * transactions.
 *
 * Handling of audio data and commands must be done by the HAL module on a
 * dedicated thread with high priority, for all modes, including MMap No
 * IRQ. The HAL module is responsible for creating this thread and setting its
 * priority. The HAL module is also responsible for serializing access to the
 * internal components of the stream while serving commands invoked via the
 * stream's AIDL interface and commands invoked via the command queue of the
 * descriptor.
 *
 * There is a state machine defined for the stream, which executes on the
 * thread handling the commands from the queue. The state machine is defined
 * in the `stream-sm.gv` file. The full list of states and commands is defined
 * by constants. Note that the 'CLOSED' state does not have a constant in the
 * interface because the client can never observe a stream with a functioning
 * command queue in this state.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable StreamDescriptor {
    /**
     * Position binds together a position within the stream and time.
     *
     * The timestamp must use "monotonic" clock.
     *
     * The frame count must advance between consecutive I/O operations, and stop
     * advancing when the stream was put into the 'standby' mode. On exiting the
     * 'standby' mode, the frame count must not reset, but continue counting.
     */
    @VintfStability
    @FixedSize
    parcelable Position {
        /** Frame count. */
        long frames;
        /** Timestamp in nanoseconds. */
        long timeNs;
    }

    /**
     * The initial state of the stream (entered after opening). When the stream
     * is in this state, it is assumed that all intermediate audio buffers are
     * empty. The HAL module must not account for any under- or overruns as the
     * client is not expected to perform audio I/O. Audio hardware may still be
     * using power in this state.
     */
    const int STATE_IDLE = 1;
    /**
     * An extension of the IDLE state which allows the HAL module to put
     * associated hardware into "standby" mode to save power.
     */
    const int STATE_STANDBY = 2;
    /**
     * The active state of the stream in which it handles audio I/O. The HAL
     * module can assume that the audio I/O will be periodic, thus inability
     * of the client to provide or consume audio data on time must be considered
     * as an under- or overrun.
     */
    const int STATE_READY = 3;
    /**
     * The ERROR state is entered when the stream has encountered an
     * irrecoverable error from the lower layer. After entering it, the stream
     * can only be closed.
     */
    const int STATE_ERROR = 4;
    /**
     * This state applies to output streams only. This is an intermediate state
     * in which the audio output has been put on hold. This state is similar to
     * the IDLE state, except that the buffers still contain audio data, thus
     * the audio pipeline is ready to continue seamlessly.
     */
    const int STATE_PAUSED = 64;
    /**
     * This state applies only to output streams opened in non-blocking mode.
     * This is a transit state which the stream takes after the control thread
     * returns control to the client for the BURST operation. On leaving this
     * state, the control thread notifies the client of the I/O completion via
     * IStreamCallback interface.
     */
    const int STATE_TRANSFER = 65;
    /**
     * This state applies only to output streams opened in non-blocking mode.
     * This is a transit state which the stream takes after the control thread
     * returns control to the client for the DRAIN operation.
     *
     * While the stream remains in this state, the HAL module must send periodic
     * updates on the remaining playback time via IStreamCallback.onDrainUpdate
     * method. The stream transfers to the IDLE state once intermediate buffers
     * are empty.
     */
    const int STATE_DRAIN = 66;

    /**
     * The START command is used to draw the stream out of IDLE or STANDBY state
     * and prepare it for audio I/O. The 'fmqByteCount' field must always be set
     * to 0.
     */
    const int COMMAND_START = 1;
    /**
     * The BURST command used for audio I/O, see 'AudioBuffer'. Differences for
     * the MMap No IRQ mode:
     *
     *  - this command only provides updated positions and latency because
     *    actual audio I/O is done via the 'AudioBuffer.mmap' shared buffer.
     *    The client does not synchronize reads and writes into the buffer
     *    with sending of this command.
     *
     *  - the 'fmqByteCount' must always be set to 0.
     */
    const int COMMAND_BURST = 2;
    /**
     * The DRAIN command is used to return the stream back to the IDLE state.
     * For the MMap no IRQ mode this command means that exchange of audio via
     * the 'AudioBuffer.mmap' shared buffer must cease until receiving the START
     * command. For output streams, all remaining data from audio buffers is
     * sent to hardware for playback (for discarding the data, use the sequence
     * of PAUSE and FLUSH commands). The 'fmqByteCount' field must always be set
     * to 0.
     */
    const int COMMAND_DRAIN = 3;
    /**
     * The STANDBY command is used to hint the HAL module that the client is
     * not going to perform audio I/O for some time, thus the HAL module can put
     * any connected hardware into standby mode to save power. The
     * 'fmqByteCount' field must always be set to 0.
     *
     * It's left on the discretion of the HAL implementation to assess all the
     * necessary conditions that could prevent hardware from being suspended,
     * and ignore the call in this case. Inability to suspend the hardware is
     * not considered as an error, in this case the stream remains in the IDLE
     * state.
     */
    const int COMMAND_STANDBY = 4;
    /**
     * The PAUSE command is used for temporarily pausing audio I/O without
     * resetting intermediate audio buffers. It puts the stream into PAUSED
     * state. The following commands can be used to leave this state:
     *
     *  - the START command resumes audio I/O;
     *
     *  - the FLUSH command resets all intermediate buffers and brings
     *    the stream into the IDLE state;
     *
     *  - the DRAIN command sends any remaining data to hardware, then also
     *    brings the stream ino the IDLE state.
     *
     * The 'fmqByteCount' field must always be set to 0 for this command.
     */
    const int COMMAND_PAUSE = 5;
    /**
     * This command is used to discard all audio data remaining in buffers after
     * pausing, and bring the stream back to the IDLE state. The 'fmqByteCount'
     * field must always be set to 0.
     *
     * It is allowed to issue FLUSH commands while the stream is in IDLE or
     * STANDBY states. The HAL module must return updated timestamps and the
     * current latency value, if possible. The FLUSH command does not take the
     * stream out of standby.
     */
    const int COMMAND_FLUSH = 6;

    /**
     * Used for sending commands to the HAL module. The client writes into
     * the queue, the HAL module reads. The queue can only contain a single
     * command.
     */
    @VintfStability
    @FixedSize
    parcelable Command {
        /**
         * One of COMMAND_* codes.
         */
        int code;
        /**
         * This field is only used for the BURST command. For all other commands
         * it must be set to 0. The following description applies to the use
         * of this field for the BURST command.
         *
         * For output streams: the amount of bytes that the client requests the
         *   HAL module to read from the 'audio.fmq' queue.
         * For input streams: the amount of bytes requested by the client to
         *   read from the hardware into the 'audio.fmq' queue.
         *
         * In both cases it is allowed for this field to contain any
         * non-negative number. The value 0 can be used if the client only needs
         * to retrieve current positions and latency. Any sufficiently big value
         * which exceeds the size of the queue's area which is currently
         * available for reading or writing by the HAL module must be trimmed by
         * the HAL module to the available size. Note that the HAL module is
         * allowed to consume or provide less data than requested, and it must
         * return the amount of actually read or written data via the
         * 'Reply.fmqByteCount' field. Thus, only attempts to pass a negative
         * number must be constituted as a client's error.
         */
        int fmqByteCount;
    }
    MQDescriptor<Command, SynchronizedReadWrite> command;

    /**
     * The value used for the 'Reply.latencyMs' field when the effective
     * latency can not be reported by the HAL module.
     */
    const int LATENCY_UNKNOWN = -1;

    /**
     * Used for providing replies to commands. The HAL module writes into
     * the queue, the client reads. The queue can only contain a single reply,
     * corresponding to the last command sent by the client.
     */
    @VintfStability
    @FixedSize
    parcelable Reply {
        /**
         * One of Binder STATUS_* statuses:
         *  - STATUS_OK: the command has completed successfully;
         *  - STATUS_BAD_VALUE: invalid value in the 'Command' structure;
         *  - STATUS_INVALID_OPERATION: the command is not applicable in the
         *                              current state of the stream, or to this
         *                              type of the stream;
         *  - STATUS_NO_INIT: positions can not be reported because the mix port
         *                    is not connected to any producer or consumer, or
         *                    because the HAL module does not support positions
         *                    reporting for this AudioSource (on input streams).
         *  - STATUS_NOT_ENOUGH_DATA: a read or write error has
         *                            occurred for the 'audio.fmq' queue;
         */
        int status;
        /**
         * Used with the BURST command only.
         *
         * For output streams: the amount of bytes actually consumed by the HAL
         *   module from the 'audio.fmq' queue.
         * For input streams: the amount of bytes actually provided by the HAL
         *   in the 'audio.fmq' queue.
         *
         * The returned value must not exceed the value passed in the
         * 'fmqByteCount' field of the corresponding command or be negative.
         */
        int fmqByteCount;
        /**
         * It is recommended to report the current position for any command.
         * If the position can not be reported, the 'status' field must be
         * set to 'NO_INIT'.
         *
         * For output streams: the moment when the specified stream position
         *   was presented to an external observer (i.e. presentation position).
         * For input streams: the moment when data at the specified stream position
         *   was acquired (i.e. capture position).
         *
         * The observable position must never be reset by the HAL module.
         * The data type of the frame counter is large enough to support
         * continuous counting for years of operation.
         */
        Position observable;
        /**
         * Used only for MMap streams to provide the hardware read / write
         * position for audio data in the shared memory buffer 'audio.mmap'.
         */
        Position hardware;
        /**
         * Current latency reported by the hardware. It is recommended to
         * report the current latency for any command. If the value of latency
         * can not be determined, this field must be set to 'LATENCY_UNKNOWN'.
         */
        int latencyMs;
        /**
         * One of the STATE_* constants indicating the state that the stream
         * was in while the HAL module was sending the reply.
         */
        int state;
    }
    MQDescriptor<Reply, SynchronizedReadWrite> reply;

    /**
     * The size of one frame of audio data in bytes. For PCM formats this is
     * usually equal to the size of a sample multiplied by the number of
     * channels used. For encoded bitstreams encapsulated into PCM the sample
     * size of the underlying PCM stream is used. For encoded bitstreams that
     * are passed without encapsulation, the frame size is usually 1 byte.
     */
    int frameSizeBytes;
    /**
     * Total buffer size in frames. This applies both to the size of the 'audio.fmq'
     * queue and to the size of the shared memory buffer for MMap No IRQ streams.
     * Note that this size may end up being slightly larger than the size requested
     * in a call to 'IModule.openInputStream' or 'openOutputStream' due to memory
     * alignment requirements.
     */
    long bufferSizeFrames;

    /**
     * Used for sending or receiving audio data to/from the stream. In the case
     * of MMap No IRQ streams this structure only contains the information about
     * the shared memory buffer. Audio data is sent via the shared buffer
     * directly.
     */
    @VintfStability
    union AudioBuffer {
        /**
         * The fast message queue used for BURST commands in all modes except
         * MMap No IRQ. Both reads and writes into this queue are non-blocking
         * because access to this queue is synchronized via the 'command' and
         * 'reply' queues as described below. The queue nevertheless uses
         * 'SynchronizedReadWrite' because there is only one reader, and the
         * reading position must be shared.
         *
         * For output streams the following sequence of operations is used:
         *  1. The client writes audio data into the 'audio.fmq' queue.
         *  2. The client writes the BURST command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  3. The high priority thread in the HAL module wakes up due to 2.
         *  4. The HAL module reads the command and audio data.
         *  5. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *  6. The client wakes up due to 5. and reads the reply.
         *
         * For input streams the following sequence of operations is used:
         *  1. The client writes the BURST command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  2. The high priority thread in the HAL module wakes up due to 1.
         *  3. The HAL module writes audio data into the 'audio.fmq' queue.
         *  4. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *  5. The client wakes up due to 4.
         *  6. The client reads the reply and audio data.
         */
        MQDescriptor<byte, SynchronizedReadWrite> fmq;
        /**
         * MMap buffers are shared directly with the DSP, which operates
         * independently from the CPU. Writes and reads into these buffers are
         * not synchronized with 'command' and 'reply' queues. However, the
         * client still uses the 'BURST' and 'DRAIN' commands for controlling the
         * audio data exchange and for obtaining current positions and latency
         * from the HAL module.
         */
        MmapBufferDescriptor mmap;
    }
    AudioBuffer audio;
}
