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
 * NOTE:
 * Was expecting to reuse android.media.audio types... Limit info to minimum to prevent
 * duplicating aidl_api. Will follow up if AudioGainConfig is exposed by android.media AIDL API.
 */
@VintfStability
parcelable AudioGainConfigInfo {
    /**
     * The identifier for the audio zone the audio device port associated to this gain belongs to.
     *
     */
    int zoneId;

    /**
     * The Audio Device Port Address.
     *
     * This is the address that can be retrieved at JAVA layer using the introspection
     * {@link android.media.AudioManager#listAudioDevicePorts} API then
     * {@link audio.media.AudioDeviceInfo#getAddress} API.
     *
     * At HAL layer, it corresponds to audio_port_v7.audio_port_device_ext.address.
     *
     * Devices that does not have an address will indicate an empty string "".
     */
    String devicePortAddress;

    /**
     * The Audio Device Port Type (e.g. BUS)
     *
     * In order to avoid confusion between native and java backends, it correspond to internal
     * device type at JAVA layer retrieved from {@link audio.media.AudioDeviceInfo#getInternalType}
     * API. It must be one of {@link AudioSystem#DEVICE_OUT_XXX}.
     *
     * At HAL layer, it corresponds to audio_devices_t, which must be in sync with AudioSystem
     * definitions.
     */
    int devicePortType;

    /**
     * The Audio Device Port Handle Id: The system unique device ID
     * The AudioPortHandle is the unique id assigned by AudioPolicyManager on device detection
     * (either always available or on setDeviceConnectionState).
     *
     * At JAVA layer, it is retrieved from {@link audio.media.AudioDeviceInfo#getId}
     * At HAL layer, it corresponds to audio_port_handle_t.
     * The 2 above definitions must be in sync.
     */
    int devicePortHandleId;

    /**
     * UI Index of the corresponding AudioGain in AudioPort.gains.
     */
    int volumeIndex;
}
