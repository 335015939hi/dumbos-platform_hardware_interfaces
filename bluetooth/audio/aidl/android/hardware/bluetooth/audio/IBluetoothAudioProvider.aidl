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

import android.hardware.bluetooth.audio.A2dpConfiguration;
import android.hardware.bluetooth.audio.A2dpConfigurationHint;
import android.hardware.bluetooth.audio.A2dpRemoteCapabilities;
import android.hardware.bluetooth.audio.A2dpStatus;
import android.hardware.bluetooth.audio.AudioConfiguration;
import android.hardware.bluetooth.audio.AudioContext;
import android.hardware.bluetooth.audio.BluetoothAudioStatus;
import android.hardware.bluetooth.audio.CodecId;
import android.hardware.bluetooth.audio.CodecParameters;
import android.hardware.bluetooth.audio.CodecSpecificCapabilitiesLtv;
import android.hardware.bluetooth.audio.CodecSpecificConfigurationLtv;
import android.hardware.bluetooth.audio.ConfigurationFlags;
import android.hardware.bluetooth.audio.IBluetoothAudioPort;
import android.hardware.bluetooth.audio.LatencyMode;
import android.hardware.bluetooth.audio.LeAudioAseConfiguration;
import android.hardware.bluetooth.audio.MetadataLtv;
import android.hardware.bluetooth.audio.Phy;
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

    /**
     * Validate and parse an A2DP Configuration,
     * shall be used with A2DP session types
     *
     * @param codecId Identify the codec
     * @param The configuration as defined by the A2DP's `Codec Specific
     *        Information Elements`, or `Vendor Specific Value` when CodecId
     *        format is set to `VENDOR`.
     * @param codecParameters result of parsing, when the validation succeeded.
     * @return A2DP Status of the parsing
     */
    A2dpStatus parseA2dpConfiguration(
            in CodecId codecId, in byte[] configuration, out CodecParameters codecParameters);

    /**
     * Return a configuration, from a list of remote Capabilites,
     * shall be used with A2DP session types
     *
     * @param remoteCapabilities The capabilities of the remote device
     * @param hint Hint on selection (audio context and/or codec)
     * @return The requested configuration. A null value value is returned
     *         when no suitable configuration has been found.
     */
    @nullable A2dpConfiguration getA2dpConfiguration(
            in List<A2dpRemoteCapabilities> remoteA2dpCapabilities, in A2dpConfigurationHint hint);

    /**
     * Set specific codec priority
     *
     *  It should be assumed that the external module will start with all its
     *  integrated codecs priority 0 by default.
     *
     * @param codecId:  codecId
     * @param priority: 0 for no priority, -1 for codec disabled,
     *				    1 - n priority, where 1 is highiest.
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
        CodecSpecificCapabilitiesLtv[] codecSpecificCapabilities;
        /**
         * Vondor codec specific capabilities.
         *
         * This will not be parsed by the BT stack.
         */
        byte[] vendorCodecSpecificCapabilities;
        /**
         * Audio capabilities metadata, packed as LTV.
         */
        @nullable MetadataLtv[] metadata;
    }

    @VintfStability
    parcelable LeAudioDataPathConfiguration {
        /**
         * Vendor specific data path identifier
         */
        int dataPathId;

        /**
         * Used in the HCI_LE_Setup_ISO_Data_Path (0x006E)
         */
        @VintfStability
        parcelable IsoDataPathConfiguration {
            /**
             * Codec ID - Valid Codec Identifier matching the selected codec
             */
            CodecId codecId;
            /**
             * Whether the transparent air mode should be set as a coding format
             * in the HCI_LE_Setup_ISO_Data_Path command, indicating that the
             * codec is not in the controller.
             *
             * If set to true, 0x03 (transparent air mode) will be used as a
             * Codec_ID coding format and the `byte[] configuration` field shall
             * remain empty. Otherwise the Codec_ID field will be set to
             * according to BT specification (0xFF coding format, company ID,
             * codec ID for vendor codecs, or according to Codec_ID identifiers
             * defined in the Assigned Numbers for the non-vendor codecs).
             */
            boolean isTransparent;
            /**
             * Controller delay
             */
            int controllerDelay;
            /**
             * Codec specific LE Audio ISO data path configuration
             * must be zero when codec ID is 0x03 transparent
             */
            byte[] configuration;
        }

        /**
         * Used in HCI_Configure_Data_Path (0x0083)
         */
        @VintfStability
        parcelable DataPathConfiguration {
            /**
             * Vendor specific data path configuration
             */
            byte[] configuration;
        }
        /**
         * Data path configuration
         */
        DataPathConfiguration dataPathConfiguration;
        /**
         * ISO data path configuration
         */
        IsoDataPathConfiguration isoDataPathConfiguration;
    }

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
        Framing framing;
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
    @Backing(type="byte")
    enum Packing {
        SEQUENTIAL = 0x00,
        INTERLEAVED = 0x01,
    }

    @VintfStability
    @Backing(type="byte")
    enum Framing {
        UNFRAMED = 0x00,
        FRAMED = 0x01,
    }

    @VintfStability
    parcelable LeAudioAseConfigurationSetting {
        /**
         * Audio Context that this configuration apply to
         */
        AudioContext audioContext;
        /**
         * Sequential or interleave packing used in Set CIG Parameters command
         */
        Packing packing;

        @VintfStability
        parcelable AseDirectionConfiguration {
            /**
             * Channel count - number of channels in CIS for an ASE
             */
            int channelCount;
            /**
             * ASE configuration
             */
            LeAudioAseConfiguration aseConfiguration;
            /**
             * QoS Configuration
             */
            @nullable LeAudioAseQosConfiguration qosConfiguration;
            /**
             * Data path configuration
             */
            LeAudioDataPathConfiguration dataPathConfiguration;
        }
        /**
         * Sink ASEs configuration
         */
        @nullable AseDirectionConfiguration sinkAseConfiguration;
        /**
         * Source ASEs configuration
         */
        @nullable AseDirectionConfiguration sourceAseConfiguration;
        /**
         * Additional flags, used for configurations with special features
         */
        @nullable ConfigurationFlags[] flags;
    }

    /**
     * ASE configuration requirements set by the BT stack.
     */
    @VintfStability
    parcelable LeAudioConfigurationRequirement {
        /**
         * Audio Contect that this requirements apply to
         */
        AudioContext audioContext;

        @VintfStability
        parcelable AseDirectionRequirement {
            /**
             * Channel count
             */
            int channelCount;
            /**
             * Optional ASE configurations requirements
             *
             * Note that the Host can set as many or as little parameters in
             * the `aseConfiguration.codecConfiguration` field as needed, to
             * closely or loosely specify the requirements. If any parameter
             * is not specified, the offloader can choose it freely. The
             * offloader should put all the specified parameters into the
             * `aseConfiguration.codecConfiguration` field of the returned
             * configuration to let the BT stack verify if the requirements
             * were met.
             */
            @nullable LeAudioAseConfiguration aseConfiguration;
        }
        /**
         * Sink ASEs configuration setting
         */
        @nullable AseDirectionRequirement sinkAseRequirement;
        /**
         * Source ASEs configuration setting
         */
        @nullable AseDirectionRequirement sourceAseRequirement;
        /**
         * Additional flags, used to request configurations with special
         * features
         */
        @nullable ConfigurationFlags[] flags;
    }

    /**
     * Method that returns a proposed ASE configuration settings for each
     * requested audio context type
     *
     * Note: _ENCODING session provides SINK ASE configuration
     *       and _DECODING session provides SOURCE ASE configuration unless
     *       BluetoothAudioProvider sets supportsMultidirectionalCapabilities to
     *       true in ProviderInfo.
     *       If supportsMultidirectionalCapabilities is set to true then the
     *       BluetoothStack expects to get configuration list for SINK and SOURCE
     *       on either _ENCODING or _DECODING session.
     *
     * @param remoteSinkAudioCapabilities List of remote sink capabilities
     *        supported by an active group devices.
     * @param remoteSourceAudioCapabilities List of remote source capabilities
     *        supported by an active group devices.
     * @param requirements ASE configuration requirements
     *
     * @return List<LeAudioAseConfigurationSetting>
     */
    List<LeAudioAseConfigurationSetting> getLeAudioAseConfiguration(
            in @nullable List<LeAudioDeviceCapabilities> remoteSinkAudioCapabilities,
            in @nullable List<LeAudioDeviceCapabilities> remoteSourceAudioCapabilities,
            in List<LeAudioConfigurationRequirement> requirements);

    @VintfStability
    parcelable LeAudioAseQosConfigurationRequirement {
        /**
         * Audio Contect Type that this requirements apply to
         */
        AudioContext contextType;

        @VintfStability
        parcelable AseQosDirectionRequirement {
            /** QoS preferences received in Codec Configured ASE state */
            Phy preferredPhy;
            int preferredRetransmissionNum;
            int maxTransportLatency;
            int presentationDelayMin;
            int presentationDelayMax;
            int preferredPresentationDelayMin;
            int preferredPresentationDelayMax;

            /**
             * ASE configuration
             */
            LeAudioAseConfiguration aseConfiguration;
        }
        /**
         * Sink ASEs configuration setting
         */
        @nullable AseQosDirectionRequirement sinkAseQosRequirement;
        /**
         * Source ASEs configuration setting
         */
        @nullable AseQosDirectionRequirement sourceAseQosRequirement;
        /**
         * Additional configuration flags requirements
         */
        @nullable ConfigurationFlags[] flags;
    }

    /**
     * A directional pair for QoS configuration. Either one or both directions
     * can be set, depending on the audio context and the requirements provided
     * to getLeAudioAseQosConfiguration().
     */
    @VintfStability
    parcelable LeAudioAseQosConfigurationPair {
        @nullable LeAudioAseQosConfiguration sinkQosConfiguration;
        @nullable LeAudioAseQosConfiguration sourceQosConfiguration;
    }

    /**
     * Method that returns an ASE QoS configuration settings for the given ASE
     * configuration,taking an ASE preferenced QoS parameters. It should be used
     * to negotiaite the QoS parameters, when the initialy received QoS
     * parameters are not within the boundaries received from the remote device
     * after configuring the ASEs.
     *
     * @param qosRequirement ASE QoS configurations requirements
     *
     * @return LeAudioAseQosConfigurationPair
     */
    LeAudioAseQosConfigurationPair getLeAudioAseQosConfiguration(
            in LeAudioAseQosConfigurationRequirement qosRequirement);

    /**
     * Broadcast quality index
     */
    @VintfStability
    @Backing(type="byte")
    enum BroadcastQuality {
        QUALITY_STANDARD,
        QUALITY_HIGH,
    }

    /**
     * It is used in LeAudioBroadcastConfigurationRequirement
     */
    @VintfStability
    parcelable LeAudioBroadcastSubgroupConfigurationRequirement {
        /**
         * Streaming Audio Context for the given subgroup.
         * This can serve as a hint for selecting the proper configuration by
         * the offloader.
         */
        AudioContext context;
        /**
         * Streaming Broadcast Audio Quality
         */
        BroadcastQuality quality;
        /**
         * Number if BISes for the given subgroup
         */
        int bisNumPerSubgroup;
    }

    /**
     * It is used in getLeAudioBroadcastConfiguration method
     * If any group id is provided, the Provider should check Pacs capabilities
     * of the group(s) and provide Broadcast configurationsupported by the
     * group.
     */
    @VintfStability
    parcelable LeAudioBroadcastConfigurationRequirement {
        List<LeAudioBroadcastSubgroupConfigurationRequirement> subgroupConfigurationRequirements;
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
         * Codec configuration for BIS or group of BISes. This shall contain all
         * the LTVs but allocation. Audio Channel Allocation will be added by
         * the Bluetooth stack unless DONT_USE_AUDIO_ALLOCATIONS flag is set in
         * the configuration returned by the offloader. This will also be used
         * to verify the requirements on the known LTV types.
         */
        CodecSpecificConfigurationLtv[] codecConfiguration;

        /**
         * Vendor specific codec configuration.
         * This will not be parsed by stack but will be used as the codec
         * specific configuration. If this is populated, only
         * `vendorCodecConfiguration` will be used, otherwise
         * `codecConfiguration` will be used. Vendor shall put any parameters
         * defined by the Assigned Numbers it decided to reuse for this
         * particular vendor codec configuration if it uses at least one vendor
         * specific parameter.
         */
        byte[] vendorCodecConfiguration;

        /**
         * Metadata for a particular BIS or group of BISes. This is optional.
         */
        @nullable MetadataLtv[] metadata;
    }

    /**
     * Subgroup BIS configuration
     *
     */
    @VintfStability
    parcelable LeAudioSubgroupBisConfiguration {
        int numBis;
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
     * LeAudioBroadcastConfigurationSetting is a result of getLeAudioBroadcastConfiguration
     * Used in HCI_LE_Create_BIG  (0x0068) command and for creating the Broadcast
     * Announcements.
     *
     */
    @VintfStability
    parcelable LeAudioBroadcastConfigurationSetting {
        /**
         * BIG parameters
         */
        int SduInterval;
        int numBis;
        int maxSdu;
        int maxTransportLatency;
        int retransmitionNum;
        Phy phy;
        Packing packing;
        Framing framing;

        /**
         * Data path configuration
         */
        LeAudioDataPathConfiguration dataPathConfiguration;

        /**
         * A list of subgroup configurations in the broadcast.
         */
        List<LeAudioBroadcastSubgroupConfiguration> subgroupsConfigurations;
    }

    /**
     * Get Broadcast configuration. Output of this function will be used
     * in HCI_LE_Create_BIG  (0x0068) command and also to create BIG INFO
     *
     */
    LeAudioBroadcastConfigurationSetting getLeAudioBroadcastConfiguration(
            in @nullable List<LeAudioDeviceCapabilities> remoteSinkAudioCapabilities,
            in LeAudioBroadcastConfigurationRequirement requirement);
}
