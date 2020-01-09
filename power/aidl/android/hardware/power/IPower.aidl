/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.power;

import android.hardware.power.Mode;

@VintfStability
interface IPower {
    /**
     * setMode() is called to enable/disable specific hint mode, which
     * may result in adjustment of power/performance parameters of the
     * cpufreq governor and other controls on device side.
     *
     * A particular platform may choose to ignore any mode hint.
     *
     * @param mode Mode which is to be enable/disable.
     * @param enabled true to enable, false to disable the mode.
     */
    oneway void setMode(in Mode mode, in boolean enabled);

    /**
     * supportMode() is called to query if the given mode hint is
     * supported by vendor. When returns false, enable the mode will have
     * no effect on the platform.
     *
     * @return true if the hint passed is supported on this device.
     * @param mode Mode to be queried
     */
    boolean supportMode(in Mode mode);

   /**
     * setUserInteractionBoost() indicates when the user is interacting with the
     * device. For example, they may be touching the screen. Note that this is
     * different from setInteractive(), which only indicates that such
     * interaction *may* occur, not that it is actively occurring. Since when
     * the user is interacting with the device, it is likely that the CPU and
     * GPU load may increase, so it may be appropriate to raise the frequencies
     * of the CPU, GPU, memory buses, etc.
     *
     * A particular platform may choose to ignore this hint.
     *
     * @param durationMs The expected duration of the user's interaction, if
     *        known, or 0 if the expected duration is unknown.
     */
    oneway void setUserInteractionBoost(in int durationMs);

   /**
     * supportUserInteractionBoost() returns whether device supports
     * setUserInteractionBoost()
     *
     * @return Whether setUserInteractionBoost() is supported.
     *         If false, calling setUserInteractionBoost() will have no effect.
     */
    boolean supportUserInteractionBoost();
}
