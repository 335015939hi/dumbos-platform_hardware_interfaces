/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
1 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.thermal;

import android.hardware.thermal.ThermalStatusCode;

@VintfStability
parcelable CpuUsage {
    /**
     * Name of this CPU.
     * All CPUs must have a different "name".
     */
    String name;

    /**
     * Active time since the last boot in ms.
     */
    long activeMs;

    /**
     * Total time since the last boot in ms.
     */
    long totalMs;

    /**
     * Is set to true when a core is online.
     * If the core is offline, all other members except |name| should be ignored.
     */
    boolean isOnline;
}
