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

import android.hardware.power.Boost;
import android.hardware.power.Hint;
import android.hardware.power.Mode;
import android.hardware.power.ThreadHint;

@VintfStability
interface IPower {
    /**
     * setMode() is called to enable/disable specific hint mode, which
     * may result in adjustment of power/performance parameters of the
     * cpufreq governor and other controls on device side.
     *
     * A particular platform may choose to ignore any mode hint.
     *
     * @param type Mode which is to be enable/disable.
     * @param enabled true to enable, false to disable the mode.
     */
    oneway void setMode(in Mode type, in boolean enabled);

    /**
     * isModeSupported() is called to query if the given mode hint is
     * supported by vendor.
     *
     * @return true if the hint passed is supported on this platform.
     *         If false, setting the mode will have no effect.
     * @param type Mode to be queried
     */
    boolean isModeSupported(in Mode type);

    /**
     * setBoost() indicates the device may need to boost some resources, as the
     * the load is likely to increase before the kernel governors can react.
     * Depending on the boost, it may be appropriate to raise the frequencies of
     * CPU, GPU, memory subsystem, or stop CPU from going into deep sleep state.
     * A particular platform may choose to ignore this hint.
     *
     * @param type Boost type which is to be set with a timeout.
     * @param durationMs The expected duration of the user's interaction, if
     *        known, or 0 if the expected duration is unknown.
     *        a negative value indicates canceling previous boost.
     *        A given platform can choose to boost some time based on durationMs,
     *        and may also pick an appropriate timeout for 0 case.
     */
    oneway void setBoost(in Boost type, in int durationMs);

    /**
     * isBoostSupported() is called to query if the given boost hint is
     * supported by vendor. When returns false, set the boost will have
     * no effect on the platform.
     *
     * @return true if the hint passed is supported on this platform.
     *         If false, setting the boost will have no effect.
     * @param type Boost to be queried
     */
    boolean isBoostSupported(in Boost type);

    /**
     * isHintSupported() is called to query if the given hint is supported by
     * the device. If this method returns false, calling the corresponding hint
     * will have no effect on the device.
     *
     * @return True if the hint passed is supported on this device.
     * @param hint Hint to be queried
     */
    boolean isHintSupported(in Hint hint);

    /**
     * setWorkloadPeriod() notifies the system of the intended period of
     * repeating work in the given process.
     *
     * One intended use of this is for games to provide their rendering rate
     * to the system so that it may align its load measurement periods with
     * that rate, but this API explicitly does not interact directly with the
     * actual display pipeline.
     *
     * Support may be queried by calling supportHint(WORKLOAD_PERIOD).
     *
     * @param pgid The process group ID of the caller
     * @param workloadPeriodMicros Intended repeating workload period in
     *        microseconds
     */
    oneway void setWorkloadPeriod(in int pgid, in int workloadPeriodMicros);

    /**
     * notifyLoadChanged() notifies the system that the top app is anticipating
     * a change in load. This change may either be in the direction of more
     * utilization (> 1.0) or in the direction of less utilization (< 1.0),
     * and is specified as relative to the current workload.
     *
     * Support may be queried by calling supportHint(LOAD_CHANGED).
     *
     * @param pgid The process group ID of the caller
     * @param cpuChange The anticipated change in CPU load
     * @param gpuChange The anticipated change in GPU load
     */
    oneway void notifyLoadChanged(in int pgid, in float cpuChange, in float gpuChange);

    /**
     * setThreadHints() notifies the system that the given thread has (or no
     * longer has) particular behaviors defined by the corresponding
     * ThreadHints.
     *
     * Support may be queried by calling supportHint(THREAD_HINTS).
     *
     * @param tid The tid of the thread for which the hints are being changed.
     * @param hintMask A bitmask of hints corresponding to the thread.
     */
    oneway void setThreadHints(in int tid, in ThreadHint hintMask);
}
