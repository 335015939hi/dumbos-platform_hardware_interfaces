/**
 *
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.tetheroffload.config;

<<<<<<< HEAD
=======
import android.os.ParcelFileDescriptor;
import android.hardware.tetheroffload.control.Status;

>>>>>>> e600a3416... Add autoconverted AIDL HALs for tetheroffload.
/**
 * Interface used for configuring the hardware management process
 */
@VintfStability
interface IOffloadConfig {
<<<<<<< HEAD
    // FIXME: AIDL does not allow boolean to be an out parameter.
    // Move it to return, or add it to a Parcelable.
    // FIXME: AIDL does not allow String to be an out parameter.
    // Move it to return, or add it to a Parcelable.
=======
>>>>>>> e600a3416... Add autoconverted AIDL HALs for tetheroffload.
    /**
     * Provides bound netlink file descriptors for use in the management process
     *
     * @param fd1   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY).
     * @param fd2   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY).
     *
<<<<<<< HEAD
     * @param out success true if successful, false otherwise
     * @param out errMsg a human readable string if eror has occured.
     */
    void setHandles(in android.os.NativeHandle fd1, in android.os.NativeHandle fd2,
        out boolean success, out String errMsg);
}
=======
     * @param out status @Status
     */
    void setHandles(in ParcelFileDescriptor fd1, in ParcelFileDescriptor fd2,
        out Status status);
}
>>>>>>> e600a3416... Add autoconverted AIDL HALs for tetheroffload.
