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

import android.hardware.bluetooth.audio.AudioCapabilities;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecInfo;
import android.hardware.bluetooth.audio.CodecParameters;
import android.hardware.bluetooth.audio.IBluetoothAudioProvider;
import android.hardware.bluetooth.audio.SessionType;

/**
 * This factory allows a HAL implementation to be split into multiple
 * independent providers.
 *
 * When the Bluetooth stack is ready to create an audio session, it must first
 * obtain the IBluetoothAudioProvider for that session type by calling
 * openProvider().
 *
 */

@VintfStability
interface IBluetoothAudioProviderFactory {
    /**
     * Gets a list of audio capabilities for a session type.
     *
     * For software encoding, the PCM capabilities are returned.
     * For hardware encoding, the supported codecs and their capabilities are
     * returned.
     *
     * @param sessionType The session type (e.g.
     *    A2DP_SOFTWARE_ENCODING_DATAPATH).
     * @return A list containing all the capabilities
     *    supported by the sesson type. The capabilities is a list of
     *    available options when configuring the codec for the session.
     *    For software encoding it is the PCM data rate.
     *    For hardware encoding it is the list of supported codecs and their
     *    capabilities.
     *    If a provider isn't supported, an empty list should be returned.
     *    Note: Only one entry should exist per codec when using hardware
     *    encoding.
     */
    AudioCapabilities[] getProviderCapabilities(in SessionType sessionType);

    /**
     * Opens an audio provider for a session type. To close the provider, it is
     * necessary to release references to the returned provider object.
     *
     * @param sessionType The session type (e.g.
     *    LE_AUDIO_SOFTWARE_ENCODING_DATAPATH).
     *
     * @return provider The provider of the specified session type
     */
    IBluetoothAudioProvider openProvider(in SessionType sessionType);

    /**
     * General information relative to a provider
     * - An optionnal name
     * - A list of codecs informations
     */
    @VintfStability
    parcelable ProviderInfo {
        String name;
        CodecInfo[] codecInfos;
    }

    /**
     * Get general informations relative to a provider.
     *
     * @param sessionType Identify the provider
     * @return General information relative to the provider. The `null` value can be
     *         returned when the provider is not available
     */
    @nullable ProviderInfo getProviderInfo(in SessionType sessionType);

    /**
     * AVDTP Remote Capabilites
     */
    @VintfStability
    parcelable AvdtpRemoteCapabilities {
        /**
         * Remote Stream Endpoint identifier
         */
        int seid;

        /**
         * Codec Identifier and capabilities as defined
         * by the A2DP's `Codec Specific Information Elements`,
         * or `Vendor Specific Value` when CodecId format is set to `VENDOR`.
         */
        CodecId id;
        byte[] a2dpCapabilities;
    }

    /**
     * LTV-Formatted metadata shared with the BT controller
     */
    @VintfStability
    parcelable ControllerData {
        byte type;
        byte[] value;
    }

    /**
     * AVDTP Service Configuration
     */
    @VintfStability
    parcelable AvdtpConfiguration {
        /**
         * Remote Stream Endpoint Identifier
         */
        int remoteSeid;

        /**
         * Codec Selection and configuration, in a generic way and as defined
         * by the A2DP's `Codec Specific Information Elements`,
         * or `Vendor Specific Value` when CodecId format is set to `VENDOR`.
         */
        CodecId id;
        CodecParameters parameters;
        byte[] a2dpConfiguration;
        ControllerData[] controllerDatas;
    }

    /**
     * AVDTP Configuration Hints.
     * - The starting audio context of the session
     * - An identifier of a prefered codec, `UNKNOWN` points no preference.
     * - Genric codec parameters
     */
    @VintfStability
    parcelable AvdtpConfigHint {
        int audioContext;
        CodecId codecId;
        CodecParameters codecConfiguration;
    }

    /**
     * Return a configuration, from a list of remote Capabilites.
     *
     * @param sessionType Identify the provider
     * @param remoteCapabilities The capabilities of the remote device
     * @param hint Hint on selection (audio context and/or codec)
     * @return The requested configuration
     */
    AvdtpConfiguration getAvdtpConfiguration(in SessionType sessionType,
            in List<AvdtpRemoteCapabilities> remoteAvdtpCapabilities, in AvdtpConfigHint hint);
}
