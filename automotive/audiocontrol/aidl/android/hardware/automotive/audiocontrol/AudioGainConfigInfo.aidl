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
 * Was expecting to reuse android.media.audio types... Limit info to minimum to prevent
 * duplicating aidl_api
 */
@VintfStability
parcelable AudioGainConfigInfo {
    /**
     * Zone ID the audio port belongs to
     */
    int zoneId;

    /**
     * The Audio Device Port Address.
     */
    String deviceAddress;

    /**
     * The Audio Device Port Type (e.g. BUS)
     */
    int deviceType;

    /**
     * the AudioPortHandle is the unique id assigned by AudioPolicyManager on device detection
     * (either always available or on setDeviceConnectionState).
     */
    int portHandle;

    /**
     * Index of the corresponding AudioGain in AudioPort.gains.
     */
    int volumeIndex;
}
