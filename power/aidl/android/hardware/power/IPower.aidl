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
     * setTopApp() reports the current top app to the HAL, if the
     * app has opted into one of the dynamic performance framework APIs. If a
     * name has been set, then if the top app changes, the framework will
     * send either an empty string or the name of the new top app, depending on
     * whether the new top app has opted into the dynamic performance framework
     * APIs.
     *
     * Support may be queried by calling supportHint(TOP_APP_PACKAGE_NAME).
     *
     * @param packageName The package name of the top app, if that app has
     *        opted into one of the dynamic performance framework APIs.
     * @param pgid The process group ID of the top app
     */
    oneway void setTopApp(in String packageName, in int pgid);

    /**
     * setRenderingRate() notifies the system of the intended rendering rate of
     * the top app in frames per second.
     *
     * Note that this does not have anything to do with the actual display
     * pipeline, but is intended to allow the system to align its load
     * measurement periods with the rendering duration.
     *
     * Support may be queried by calling supportHint(RENDERING_RATE).
     *
     * @param renderingRate Intended rendering rate of the top app in frames
     *        per second.
     */
    oneway void setRenderingRate(in int renderingRate);

    /**
     * notifyLoadChanged() notifies the system that the top app is anticipating
     * a change in load. This change may either be in the direction of more
     * utilization (> 1.0) or in the direction of less utilization (< 1.0),
     * and is specified as relative to the current workload.
     *
     * Support may be queried by calling supportHint(LOAD_CHANGED).
     *
     * @param cpuChange The anticipated change in CPU load
     * @param gpuChange The anticipated change in GPU load
     */
    oneway void notifyLoadChanged(in float cpuChange, in float gpuChange);

    /**
     * setThreadHint() notifies the system that the given thread has (or no
     * longer has) a particular behavior as defined by the corresponding
     * ThreadHint.
     *
     * Support may be queried by calling supportHint(THREAD_HINT).
     *
     * @param thread The tid of the thread for which the hint is being changed.
     * @param hint The hint to set or clear.
     * @param enabled Whether to set (true) or clear (false) the hint.
     */
    oneway void setThreadHint(in int thread, in ThreadHint hint,
            boolean enabled);
}
