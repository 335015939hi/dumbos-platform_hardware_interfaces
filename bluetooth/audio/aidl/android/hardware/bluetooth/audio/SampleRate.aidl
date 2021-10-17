/*
 * Copyright 2021 The Android Open Source Project
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
enum SampleRate {
    RATE_UNKNOWN = 0x00,
    RATE_44100 = 0x01,
    RATE_48000 = 0x02,
    RATE_88200 = 0x04,
    RATE_96000 = 0x08,
    RATE_176400 = 0x10,
    RATE_192000 = 0x20,
    RATE_16000 = 0x40,
    RATE_24000 = 0x80,
    RATE_8000 = 0x100,
    RATE_32000 = 0x200,
}
