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

package android.hardware.fastboot;

import android.hardware.fastboot.FileSystemType;
import android.hardware.fastboot.Result;

/**
 * IFastboot interface implements vendor specific fastboot commands.
 */
@VintfStability
interface IFastboot {
    /**
     * Executes a fastboot OEM command.
     *
     * @param oemCmd The oem command that is passed to the fastboot HAL.
     * @return Returns the status SUCCESS if the operation is successful,
     *     INVALID_ARGUMENT for bad arguments,
     *     NOT_SUPPORTED for an unsupported command.
     */
    Result doOemCommand(in String oemCmd);

    /**
     * Executes an OEM specific erase after fastboot erase userdata.
     *
     * @return Returns the status SUCCESS if the operation is successful,
     *     NOT_SUPPORTED for unsupported command.
     *     FAILURE_UNKNOWN for unknown error in the oem specific command.
     */
    Result doOemSpecificErase();

    /**
     * Returns the minimum battery voltage required for flashing in mV.
     *
     * @param out result Returns the status SUCCESS if the operation is successful,
     *     FAILURE_UNKNOWN otherwise.
     * @return Minimum batterery voltage (in mV) required for flashing to
     *     be successful.
     */
    int getBatteryVoltageFlashingThreshold(out Result result);

    /**
     * Returns whether off-mode-charging is enabled. If enabled, the device
     *      autoboots into a special mode when power is applied.
     *
     * @param out result Returns the status SUCCESS if the operation is successful,
     *     FAILURE_UNKNOWN otherwise.
     * @return Returns whether off-mode-charging is enabled.
     */
    boolean getOffModeChargeState(out Result result);

    /**
     * Returns the file system type of the partition. This is only required for
     *      physical partitions that need to be wiped and reformatted.
     * @param in partitionName Name of the partition.
     * @param out result SUCCESS if the operation is successful,
     *     FAILURE_UNKNOWN if the partition is invalid or does not require
     *     reformatting.
     * @return Returns the file system type of the partition. Type can be ext4,
     *      f2fs or raw.
     */
    FileSystemType getPartitionType(in String partitionName, out Result result);

    /**
     * Returns an OEM-defined string indicating the variant of the device, for
     * example, US and ROW.
     * @param out result Returns the status SUCCESS if the operation is successful,
     *     FAILURE_UNKNOWN otherwise.
     * @return Indicates the device variant.
     */
    String getVariant(out Result result);
}
