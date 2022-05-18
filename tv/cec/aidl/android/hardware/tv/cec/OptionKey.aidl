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

package android.hardware.tv.cec;

/**
 * Options used for IHdmiCec.setOption()
 */
@VintfStability
@Backing(type="int")
enum OptionKey {
    /**
     * When set to false, HAL does not wake up the system upon receiving <Image
     * View On> or <Text View On>. Used when user changes the TV settings to
     * disable the auto TV on functionality.
     * True by default.
     */
    WAKEUP = 1,
    /**
     * When set to false, all the CEC commands are discarded. Used when user
     * changes the TV settings to disable CEC functionality.
     * True by default.
     */
    ENABLE_CEC = 2,
    /**
     * Setting this flag to false means Android system must stop handling CEC
     * service and yield the control over to the microprocessor that is powered
     * on through the standby mode. When set to true, the system must gain the
     * control over, hence telling the microprocessor to stop handling the CEC
     * commands. For example, this may be called when system goes in and out of
     * standby mode to notify the microprocessor that it should start/stop
     * handling CEC commands on behalf of the system.
     * False by default.
     */
    SYSTEM_CEC_CONTROL = 3,
}
