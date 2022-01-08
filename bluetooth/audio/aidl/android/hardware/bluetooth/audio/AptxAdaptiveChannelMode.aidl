/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

@VintfStability
@Backing(type="int")
enum AptxAdaptiveChannelMode {
    /** Channel Mode: 5 bits */
    UNCHANGED = -1,
    /* Joint Stereo - default mode */
    JOINT_STEREO = 0,
    /* Legacy Mono */
    MONO = 1,
    /* Split TX where we send the L to the L and R to the R */
    DUAL_MONO = 2,
    /* TWS master sink forward the stereo stream to a slave sink */
    TWS_STEREO = 4,
    /* output data in each packet is interleaved (16-bit left, 16-bit right) */
    EARBUD = 8,
    /* TWS-Legacy Mono is when a stereo stream is sent to a master sink and the
     * master sink forward a mono stream (left or right channel) to a slave sink
     */
    TWS_MONO = 10,
    UNKNOWN = 0xFF,
}
