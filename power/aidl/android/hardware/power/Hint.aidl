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

@VintfStability
@Backing(type="int")
enum Hint {
    /**
     * Whether setWorkloadPeriod() is supported, which allows an app to notify
     * the HAL of its repeating workload period (e.g., frame duration) in order
     * to better align load measurement periods.
     */
    WORKLOAD_PERIOD,

    /**
     * Whether notifyLoadChanged() is supported, which allows an app to notify
     * the HAL of an upcoming change in CPU or GPU load so the system can
     * proactively prepare rather than waiting for the load change to be
     * observed (and potentially dropping frames).
     */
    LOAD_CHANGED,

    /**
     * Whether setThreadHints() is supported, which allows an app to notify the
     * HAL that particular threads have characteristics like high utilization
     * or low latency in order to enable better scheduling and frequency
     * management.
     */
    THREAD_HINTS,
}
