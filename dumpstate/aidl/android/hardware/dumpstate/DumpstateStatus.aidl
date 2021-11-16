/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.dumpstate;

/**
 * A simple return enum for use with dumpstateBoard_1_1.
 */
@VintfStability
@Backing(type="int")
enum DumpstateStatus {
    OK = 0,
    /**
     * Returned for cases where the device doesn't support the given DumpstateMode (e.g. a phone
     * trying to use DumpstateMode::WEAR).
     */
    UNSUPPORTED_MODE = 1,
    /**
     * Returned for cases where an IllegalArgumentException is typically appropriate, e.g. missing
     * file descriptors.
     */
    ILLEGAL_ARGUMENT = 2,
    /**
     * Returned when device logging is not enabled.
     */
    DEVICE_LOGGING_NOT_ENABLED = 3,
}
