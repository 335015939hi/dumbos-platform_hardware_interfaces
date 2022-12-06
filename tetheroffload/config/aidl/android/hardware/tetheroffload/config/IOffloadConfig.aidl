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

package android.hardware.tetheroffload.config;

import android.hardware.common.NativeHandle;
import android.hardware.tetheroffload.common.Status;

/**
 * Interface used for configuring the hardware management process
 */
@VintfStability
interface IOffloadConfig {
    /**
     * Provides bound netlink file descriptors for use in the management process
     *
     * @param fd1   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_NEW | NF_NETLINK_CONNTRACK_DESTROY).
     * @param fd2   A file descriptor bound to the following netlink groups
     *              (NF_NETLINK_CONNTRACK_UPDATE | NF_NETLINK_CONNTRACK_DESTROY).
     *
     * @return Status of the call.
     */
    Status setHandles(in NativeHandle fd1, in NativeHandle fd2);
}
