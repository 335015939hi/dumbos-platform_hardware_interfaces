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
     * @param type Mode which is to be enable/disable.
     * @param enabled true to enable, false to disable the mode.
     */
    oneway void setMode(in Mode type, in boolean enabled);

    /**
     * supportMode() is called to query if the given mode hint is
     * supported by vendor. When returns false, set/unset the mode will have
     * no effect on the platform.
     *
     * @return true if the hint passed is supported on this platform.
     * @param type Mode to be queried
     */
    boolean supportMode(in Mode type);

   /**
     * setBoost() indicates the device need keep a boost on CPU and GPU, as the
     * the load is likely to increase or govornor need react refore kernel govornor,
     * The boost may be appropriate to raise the frequencies  of the CPU, GPU, memory
     * buses, or stop CPU going into deep sleep state. 
     *
     * A particular platform may choose to ignore this hint.
     *
     * @param type Boost type which is to be set with a timeout.
     * @param durationMs The expected duration of the user's interaction, if
     *        known, or 0 if the expected duration is unknown.
     *        a nagtive value indicates canceling previous boost.
     *        A given platform can chooste to boost some time based on durationMs,
     *        and also pick a appropriate timeout for 0 case.
     */
    oneway void setBoost(in Boost type, in int durationMs);

    /**
     * supportBoost() is called to query if the given boost hint is
     * supported by vendor. When returns false, set the boost will have
     * no effect on the platform.
     *
     * @return true if the hint passed is supported on this platform.
     * @param mode Mode to be queried
     */
    boolean supportBoost(in Boost type);
}
