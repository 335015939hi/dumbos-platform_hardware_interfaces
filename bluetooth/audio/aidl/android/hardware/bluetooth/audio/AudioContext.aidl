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

/**
 * Context of the audio configuration
 * As defined by PACS, and used either by A2DP and LE Audio
 */
@VintfStability
enum AudioContext {
    UNSPECIFIED = 0,
    CONVERSATIONAL = 1,
    MEDIA = 2,
    GAME = 3,
    INSTRUCTIONAL = 4,
    VOICE = 5,
    LIVE_AUDIO = 6,
    SOUND_EFFECTS = 7,
    NOTIFICATIONS = 8,
    RINGTONE = 9,
    ALERTS = 10,
    EMERGENCY_ALARM = 11
}
