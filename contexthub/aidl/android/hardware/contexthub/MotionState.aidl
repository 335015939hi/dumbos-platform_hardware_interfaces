/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.hardware.contexthub;

/**
 * Used to indicate the motion state of the device.
 */
@VintfStability
@Backing(type="byte")
enum MotionState {
    // Motion state is unspecified or unknown.
    UNKNOWN = 0;

    // Device has not seen meaningful accelerometer activity within the last 2s, and was not known
    // to be experiencing Location-changing motion within the last 5s. Detection latency: < 5s.
    STILL = 1 << 0;

    // Device has seen meaningful accelerometer activity within the last 2s, and was not known to be
    // experiencing Location-changing motion within the last 5s.
    // Detection latency: < 2s.
    LOCAL_MOTION = 1 << 1;

    // Device is known to be experiencing Location-changing motion.
    // Detection latency: < 10s
    LOCATION_MOTION = 1 << 2;
}
