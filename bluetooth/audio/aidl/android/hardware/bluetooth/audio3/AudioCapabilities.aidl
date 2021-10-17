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

package android.hardware.bluetooth.audio3;

import android.hardware.bluetooth.audio3.CodecCapabilities;
import android.hardware.bluetooth.audio3.Lc3CodecCapabilities;
import android.hardware.bluetooth.audio3.PcmParameters;

/**
 * Used to specify the capabilities of the different session types
 */
@VintfStability
union AudioCapabilities {
    PcmParameters pcmCapabilities;
    CodecCapabilities codecCapabilities;
    Lc3CodecCapabilities leAudioCapabilities;
}
