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
enum ThreadHint {
    /**
     * Informs the system that even if this thread sometimes has a light
     * workload, it is typically used for heavy CPU work, and should
     * therefore be treated as a high-utilization thread, even when lightly
     * loaded.
     */
    HIGH_UTILIZATION,

    /**
     * Informs the system that it may be desirable for the system to prioritize
     * this thread over other non-low-latency threads in order to ensure that
     * it meets deadlines, such as frame boundaries.
     */
    LOW_LATENCY,
}
