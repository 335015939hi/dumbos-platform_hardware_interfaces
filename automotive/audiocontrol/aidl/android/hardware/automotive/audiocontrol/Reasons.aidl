/*
 * Copyright (C) 2022 The Android Open Source Project
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

package android.hardware.automotive.audiocontrol;

/**
 * Enum to identify the reason(s) of
 * {@link android.hardware.automotive.audiocontrol.AudioGainConfigInfo} changed event
 */
@Backing(type="int")
@VintfStability
enum Reasons {
    CYBER_MASTER_MUTE           = 0x1,
    REMOTE_MUTE                 = 0x2,
    TCU_MUTE                    = 0x4,
    ADAS_DUCKING                = 0x8,
    NAV_DUCKING                 = 0x10,
    CAR_PLAY_ALT_DUCKING        = 0x20,
    THERMAL_LIMITATION          = 0x40,
    SUSPEND_EXIT_VOL_LIMITATION = 0x80,
    EXTERNAL_AMP_VOL_FEEDBACK   = 0x100,
    OTHER                       = 0x80000000,
}
