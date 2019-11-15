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

package android.hardware.atrace;

import android.hardware.atrace.TracingCategory;

@VintfStability
interface IAtraceDevice {

    /**
     * Get vendor extended atrace points.
     * Each returned TracingCategory should have a unique name.
     *
     * @return tracing points the device extended.
     */
    TracingCategory[] listCategories();

    /**
     * A hook when atrace set to enable specific categories, so HAL
     * can enable kernel tracing points and/or notify other things
     * for userspace tracing turning on.
     *
     * @param categories A vector of strings of categories (corresponding to
     *        TracingCategory.name) atrace needs to be enabled.
     */
    void enableCategories(in String[] categories);

    /**
     * A hook when atrace set to clean up tracing categories, so HAL
     * can disable all kernel tracing points and/or notify other things
     * for userspace tracing turning off.
     */
    void disableAllCategories();
}
