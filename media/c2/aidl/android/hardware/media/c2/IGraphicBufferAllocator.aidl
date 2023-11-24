/*
 * Copyright (C) 2023 The Android Open Source Project
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

import android.hardware.HardwareBuffer;
import android.os.ParcelFileDescriptor;

/**
 * Interface for decoder output buffer allocator for HAL process
 *
 * A graphic buffer for decoder output is allocated by the interface.
 */
@VintfStability
interface IGraphicBufferAllocator {
    /**
     * A graphic buffer allocation.
     *
     * buffer is in android.hardware.HardwareBuffer.
     * fence is provided in order to signal readiness of the buffer I/O inside
     * underlying Graphics subsystem. This is called a sync fence throughout Android framework.
     */
    parcelable Allocation {
        HardwareBuffer buffer;
        ParcelFileDescriptor fence;
    }

    /**
     * Parameters for a graphic buffer allocation.
     *
     * Refer to AHardwareBuffer_Desc(libnativewindow) for details.
     */
    parcelable Description {
        int width;
        int height;
        int format;
        long usage;
    }

    /**
     * File descriptors in order to wait for ready and/or stop events.
     *
     * file descriptors are checked whether there is events using ::poll for readable status.
     * Specfically \p ready is used for waiting until being ready to allocate.
     * \p end is used for notifying end of the life-cycle of the interface.
     */
    parcelable WaitableFds {
        ParcelFileDescriptor ready;
        ParcelFileDescriptor stop;
    }

    /**
     * Allocate a graphic buffer.
     *
     * @param desc Allocation parameters.
     * @return an android.hardware.HardwareBuffer which is basically same to
     *     AHardwareBuffer. If underlying grpahics system is blocked, c2::Status::Blocked
     *     will be returned. In this case getWaitableFds() will return file descriptors which
     *     can be used to construct a waitable object. The waitable object will be notified
     *     when underlying graphics system is unblocked
     * @throws ServiceSpecificException with one of the following values:
     *   - `c2::Status::BAD_STATE` - The client is not in running states.
     *   - `c2::Status::BLOCKED`   - Underlying graphics system is blocked.
     *   - `c2::Status::CORRUPTED` - Some unknown error occurred.
     */
    Allocation allocate(in Description desc);

    /**
     * De-allocate a graphic buffer by graphic buffer's unique id.
     *
     * @param id graphic buffer's unique id. See also AHardwareBuffer_getId().
     * @return {@code true} when de-allocate happened, {@code false} otherwise.
     */
    boolean deallocate(in long id);

    /**
     * Gets waitable file descriptors.
     *
     * Use this method once and cache fds in order not to create unnecessary duplicated fds.
     *
     * Two file descriptors are created by eventfd(). one fd is used for mirroring
     * the current # of allocatable buffers using EFD_SEMAPHORE. the other fd is used
     * for notifying the end-of-lifecycle of the interface. see the description of
     * the returned {@code parcelable}.
     *
     * The both returned file descriptors can be polled whether the read is ready
     * via ::poll(). C2Fence object should be implemented based on this Fds. C2Fence
     * may utilize ::poll() for waiting for being ready to allocate and/or end-of-life-cycle.
     *
     * If many waitable objects based on the same fd are competing, all watiable objects will be
     * notified. After being notified, they should invoke allocate(). At least one of them can
     * successfully allocate. Others not having an Allocation will have c2::Status::BLOCKED
     * as return value. They should wait again via waitable objects based on the fds which are
     * already returned from this interface.
     *
     * @return a parcelable which has two fds.
     */
    WaitableFds getWaitableFd();
}
