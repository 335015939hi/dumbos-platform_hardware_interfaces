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

import android.hardware.atrace.TracingEvent;

@VintfStability
parcelable TracingCategory {
    /**
     * Tracing category name.
     * This should be a short, lowercase string without spaces that developers
     * can use to identify this category. For example 'sched', 'gfx', or
     * 'flashlight'. This may be the same name as an existing atrace category
     * can be listed 'adb shell atrace --list'.
     */
    String name;

    /**
     * A human readable description of this category.
     * For example: "CPU Frequency and System Clock"
     */
    String description;

    /**
     * Events in this category. For example:
     */
    TracingEvent[] events;
}

