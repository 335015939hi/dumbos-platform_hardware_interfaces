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

package android.hardware.rpmb;

import android.hardware.rpmb.RpmbMode;

@VintfStability
parcelable RpmbGeometry {
    /**
     * Mode of device. Normal or Advanced
     */
    RpmbMode mode;

    /**
     * Maximum number of RPMB frames allowed in Security Protocol In and Security Protocol output
     * This is the same as the number of RPMB blocks that can be read in 1 request to the device.
     */
    byte max_rd_cnt;

    /**
     * Maximum number of RPMB frames allowed in Security Protocol In and Security Protocol output
     * This is the same as the number of RPMB blocks that can be written in 1 request to the device.
     */
    byte max_wr_cnt;

    /*
     * Capacity of the device (expressed in number of RPMB blocks) available to the client.
     * In cases where the backend is a dedicated RPMB region in device, capacity should be
     * the number of blocks available in this RPMB region.
     */
    byte capacity;
}
