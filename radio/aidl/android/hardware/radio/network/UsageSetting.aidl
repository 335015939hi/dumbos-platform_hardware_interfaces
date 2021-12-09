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

package android.hardware.radio.network;

/**
 * Cellular usage setting with values according to 3gpp 24.301 sec 4.3 and 3gpp 24.501 sec 4.3.
 */
@VintfStability
@Backing(type="int")
enum UsageSetting {
    /** UE operates in voice-centric mode */
    VOICE_CENTRIC = 1,

    /** UE operates in data-centric mode */
    DATA_CENTRIC = 2,
}
