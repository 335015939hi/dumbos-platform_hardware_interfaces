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

import android.hardware.bluetooth.audio.AudioConfiguration;
import android.hardware.bluetooth.audio.BluetoothAudioStatus;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.FeatureFlags;
import android.hardware.bluetooth.audio.IBluetoothAudioPort;
import android.hardware.bluetooth.audio.LatencyMode;
import android.hardware.bluetooth.audio.LeAudioAseConfiguration;
import android.hardware.bluetooth.audio.LeAudioConfiguration.StreamMap;
import android.hardware.bluetooth.audio.LtvData;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * HAL interface from the Bluetooth stack to the Audio HAL
 *
 * The Bluetooth stack calls methods in this interface to start and end audio
 * sessions and sends callback events to the Audio HAL.
 *
 */
@VintfStability
interface IBluetoothAudioProvider {
    /**
     * Ends the current session and unregisters the IBluetoothAudioPort
     * interface.
     */
    void endSession();

    /**
     * This method indicates that the Bluetooth stack is ready to stream audio.
     * It registers an instance of IBluetoothAudioPort with and provides the
     * current negotiated codec to the Audio HAL. After this method is called,
     * the Audio HAL can invoke IBluetoothAudioPort.startStream().
     *
     * Note: endSession() must be called to unregister this IBluetoothAudioPort
     *
     * @param hostIf An instance of IBluetoothAudioPort for stream control
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     * @param supportedLatencyModes latency modes supported by the active
     * remote device
     *
     * @return The fast message queue for audio data from/to this
     *    provider. Audio data will be in PCM format as specified by the
     *    audioConfig.pcmConfig parameter. Invalid if streaming is offloaded
     *    from/to hardware or on failure
     */
    MQDescriptor<byte, SynchronizedReadWrite> startSession(
            in IBluetoothAudioPort hostIf, in AudioConfiguration audioConfig,
            in LatencyMode[] supportedLatencyModes);
    /**
     * Callback for IBluetoothAudioPort.startStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamStarted(in BluetoothAudioStatus status);

    /**
     * Callback for IBluetoothAudioPort.suspendStream()
     *
     * @param status true for SUCCESS or false for FAILURE
     */
    void streamSuspended(in BluetoothAudioStatus status);

    /**
     * Set specific codec priority
     *
     *  It should be assumed that the external module will start with all its
     *  integrated codecs priority 0 by default.
     *
     * @param codecId: 	codecId
     * @param priority: 	0 for no priority, -1 for codec disabled,
     *				1 - n priority, where 1 is highiest.
     */
    void setCodecPriority(in CodecId codecId, int priority);

    /**
     * LE Audio device Capabilities
     */
    @VintfStability
    parcelable LeAudioDeviceCapabilities {
        /**
         * Codec Identifier
         */
        CodecId codecId;

        /**
         * Codec capabilities, packed as LTV.
         */
        LtvData[] codecSpecificCapabilities;

        /**
         * Audio capabilities metadata, packed as LTV.
         */
        @nullable LtvData[] metadata;
    }

    /**
     * Used in the HCI_LE_Setup_ISO_Data_Path (0x006E)
     */
    @VintfStability
    parcelable IsoDataPathConfiguration {
        /**
         * Codec ID - Valid Codec coding format for in-controller encoding,
         *            or 0x03 (transparent) in other cases.
         */
        CodecId codecId;

        /**
         * Vendor specific data path identifier
         */
        int dataPathId;

        /**
         * Vendor specific LE Audio ISO data path configuration
         */
        @nullable byte[] configuration;
    }

    /**
     * Used in HCI_Configure_Data_Path (0x0083)
     */
    @VintfStability
    parcelable DataPathConfiguration {
        /**
         * Vendor specific data path identifier
         */
        int dataPathId;

        /**
         * Vendor specific data path configuration
         */
        @nullable byte[] configuration;
    }

    @VintfStability
    parcelable LeAudioAseConfigurationSetting {
        /**
         * Proposed ASE configurations
         */
        LeAudioAseConfiguration aseConfiguration;

        /**
         * Additional flags, used to request configurations with special
         * features
         */
        @nullable FeatureFlags[] flags;
    }

    /**
     * ASE configuration requirements set by the BT stack.
     */
    @VintfStability
    parcelable LeAudioConfigurationRequirement {
        int contextType;
        int channelCount;

        /** Optional configuration requirement */
        @nullable LeAudioAseConfigurationSetting aseConfiguration;
    }

    /**
     * Method that returns a proposed ASE configuration
     *
     * Note: _ENCODING session provides SINK ASE configuration
     *       _DECODING session provides SOURCE ASE configuration
     *
     * @param remotePacsCapabilities List of remote capabilities supported
     *        by an active group devices.
     * @param requirement ASE configuration requirement set by the BT stack
     *
     * @return LeAudioAseConfigurationSetting
     */
    LeAudioAseConfigurationSetting getLeAudioAseConfiguration(
            in List<LeAudioDeviceCapabilities> remoteAudioCapabilities,
            in LeAudioConfigurationRequirement requirement);

    @VintfStability
    parcelable LeAudioAseQosConfiguration {
        /**
         * SDU Interval used in Set CIG Parameters command and Configure QoS
         */
        int sduInterval;

        /**
         * Flush timeout used in Set CIG Parameters command
         */
        int flushTimeout;

        /**
         * Framing used in Set CIG Parameters command and Configure QoS
         */
        int framing;

        /**
         * Max transport latency used in Set CIG Parameters command and
         * Configure QoS.
         */
        int maxTransportLatency;

        /**
         * Max SDU used in Set CIG Parameters command and Configure QoS
         */
        int maxSdu;

        /**
         * Retransmission number used in Set CIG Parameters command and
         * Configure QoS
         */
        int retransmissionNum;
    }

    @VintfStability
    parcelable LeAudioAseQosConfigurationSetting {
        /**
         * Sequential or interleave packing used in Set CIG Parameters command
         */
        int packing;

        /**
         * ASE QoS configuration
         */
        LeAudioAseQosConfiguration aseQos;
    }

    /**
     * ASE configuration hint which could help codec manager to propose
     * best configuration.
     */
    @VintfStability
    parcelable LeAudioQosConfigurationHint {
        int preferredRetransmisionNumber;
        int maxTransportLatency;

        CodecId codecId;
        LtvData[] codecConfiguration;
    }

    /**
     * Method that returns QoS configuration for ASE
     *
     * @param hint Hint for codec manager to propose best ASE configurations
     *
     * @return LeAudioGroupConfiguration
     */
    LeAudioAseQosConfigurationSetting getLeAudioAseQosConfiguration(
            in LeAudioQosConfigurationHint qosConfigurationHint);

    @VintfStability
    parcelable LeAudioDataPathConfiguration {
        /**
         * Data path configuration
         */
        DataPathConfiguration dataPathConfiguration;

        /**
         * ISO data path configuration
         */
        IsoDataPathConfiguration isoDataPathConfiguration;
    }

    /**
     * Get data path configuration
     *
     * @param streamMap as defined in LeAudioConfiguration.aidl
     */
    LeAudioDataPathConfiguration getLeAudioDataPathConfiguration(StreamMap[] streamMap);

    /**
     * It is used in LeAudioBroadcastConfigurationHint
     */
    @VintfStability
    parcelable LeAudioBroadcastSubgroupConfigurationHint {
        int contextType;
        int quality;
        int bisNumPerSubgroup;
    }

    /*
     * It is used in getLeAudioBroadcastConfiguration method
     * When any group id is provided, then Provider should check Pacs
     * capabilities of the group(s) and provide Broadcast configuration
     * supported by the group
     */
    @VintfStability
    parcelable LeAudioBroadcastConfigurationHint {
        List<LeAudioBroadcastSubgroupConfigurationHint> subgroupConfigurationHit;
    }

    /**
     * BIS configuration
     */
    @VintfStability
    parcelable LeAudioBisConfiguration {
        /**
         * Codec ID
         */
        CodecId codecId;

        /**
         * Codec configuration for BIS. This shall contain all the LTVs but
         * allocation. Audio Channel Allocation will be added by the
         * Bluetooth stack.
         */
        LtvData[] codecConfiguration;

        /**
         * Metadata for a particualar BIS subgroup. This is optional
         */
        LtvData[] metadata;
    }

    /**
     * Subgroup BIS configuration
     *
     */
    @VintfStability
    parcelable LeAudioSubgroupBisConfiguration {
        Int numBis;
        LeAudioBisConfiguration bisConfiguration;
    }

    /**
     * List of subgroups configuration
     *
     */
    @VintfStability
    parcelable LeAudioBroadcastSubgroupConfiguration {
        List<LeAudioSubgroupBisConfiguration> bisConfigurations;
    }

    /**
     * LeAudioBroadcastConfiguration is a result of getLeAudioBroadcastConfiguration
     * in HCI_LE_Create_BIG  (0x0068) command
     *
     */
    @VintfStability
    parcelable LeAudioBroadcastConfiguration {
        int SduInterval;
        int numBis;
        int maxSdu;
        int maxTransportLatency;
        int retransmitionNum;
        int phy;
        int packing;
        int framing;

        DataPathConfiguration dataPathConfiguration;
        IsoDataPathConfiguration isoDataPath;

        List<LeAudioBroadcastSubgroupConfiguration> subgroupsConfigurations;
    }

    /**
     * Get Broadcast configuration. Output of this function will be used
     * in HCI_LE_Create_BIG  (0x0068) command and also to create BIG INFO
     *
     */
    LeAudioBroadcastConfiguration getLeAudioBroadcastConfiguration(
            in List<LeAudioCapabilities> remoteAudioCapabilities,
            in LeAudioBroadcastConfigurationHint hint);

    /**
     * Called when the audio configuration of the stream has been changed.
     *
     * @param audioConfig The audio configuration negotiated with the remote
     *    device. The PCM parameters are set if software based encoding,
     *    otherwise the correct codec configuration is used for hardware
     *    encoding.
     */
    void updateAudioConfiguration(in AudioConfiguration audioConfig);

    /**
     * Called when the supported latency mode is updated.
     *
     * @param allowed If the peripheral devices can't keep up with low latency
     * mode, the API will be called with supported is false.
     */
    void setLowLatencyModeAllowed(in boolean allowed);
}
