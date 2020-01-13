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
     * @return True if the mode passed is supported on this device.
     * @param mode Mode to be queried
     */
    boolean supportMode(in Mode mode);

    /**
     * getFixedPerformanceScaleFactor() provides a scaling factor which will be
     * used when verifying the stability of fixed performance mode. The number
     * of iterations used for the test suite will be scaled by this factor,
     * with the expectation that this scaled test suite will complete in 10
     * seconds.
     *
     * If (and only if) fixed performance modes are not available on this device
     * (as reported by supportMode()), this function must return 0.
     *
     * @return The scaling factor to be used when verifying the
     *         FIXED_PERFORMANCE_MAXIMUM_SUSTAINABLE mode
     */
    int getFixedPerformanceScaleFactor();

    /**
     * supportHint() is called to query if the given hint is supported by the
     * device. If this method returns false, calling the corresponding hint
     * will have no effect on the device.
     *
     * @return True if the hint passed is supported on this device.
     * @param hint Hint to be queried
     */
    boolean supportHint(in Hint hint);

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
     * Support may be queried by calling supportHint(USER_INTERACTION_BOOST).
     *
     * @param durationMs The expected duration of the user's interaction, if
     *        known, or 0 if the expected duration is unknown.
     */
    oneway void setUserInteractionBoost(in int durationMs);

    /**
     * setDisplayUpdateImminent() indicates that the framework is likely to
     * provide a new display frame soon. This implies that the device should
     * ensure that the display processing path is powered up and ready to
     * receive that update.
     *
     * Support may be queried by calling supportHint(DISPLAY_UPDATE_IMMINENT).
     *
     * @param targetNs The time that the given update is expected to be visible
     *        on the display in absolute CLOCK_MONOTONIC nanoseconds.
     */
    oneway void setDisplayUpdateImminent(in long targetNs);

    /**
     * setTopAppPackageName() reports the current top app to the HAL, if the
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
     */
    oneway void setTopAppPackageName(in string packageName);

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
    oneway void notifyLoadChanged(cpuChange, gpuChange);
}
