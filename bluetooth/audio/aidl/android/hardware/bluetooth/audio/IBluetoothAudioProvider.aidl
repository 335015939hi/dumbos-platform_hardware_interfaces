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
     *                  1 - n priority, where 1 is highiest.
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
         * Vendor codec specific capabilities.
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

    /**
     * Connected Isochronous Channel arrangement within the Connected
     * Isochronous Group. As defined in Bluetooth Core Specification Version
     * 5.3, Vol 4, Part E, Sec. 7.8.97.
     */
    @VintfStability
    @Backing(type="byte")
    enum Packing {
        SEQUENTIAL = 0x00,
        INTERLEAVED = 0x01,
    }

    /**
     * Isochronous Data PDU framing parameter. As defined in Bluetooth Core
     * Specification Version 5.3, Vol 4, Part E, Sec. 7.8.97.
     */
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
             * Channel count - number of channels in a CIS for an ASE
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
             * Channel count - number of channels in a CIS for an ASE
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

        /**
         * QoS preferences received in Codec Configured ASE state. As defined in
         * bluetooth service specification: Audio Stream Control Service" V1.0,
         * Sec. 4.1 Audio Stream Endpoints, Table 4.3:"Additional_ASE_Parameters
         * format when ASE_State = 0x01 (Codec Configured)".
         */
        @VintfStability
        parcelable AseQosDirectionRequirement {
            /**
             * Support for unframed Isochronous Adaptation Layer PDUs.
             * When set to FRAMED, the unframed PDUs are not supported.
             */
            Framing framing;
            /**
             * Preferred value for the PHY parameter to be written by the client
             * for this ASE in the Config QoS operation
             */
            Phy[] preferredPhy;
            /**
             * Preferred value for the Retransmission Number parameter to be
             * written by the client for this ASE in the Config QoS operation.
             */
            int preferredRetransmissionNum;
            /**
             * Preferred value for the Max Transport Latency parameter to be
             * written by the client for this ASE in the Config QoS operation.
             */
            int maxTransportLatency;
            /**
             * Minimum server supported Presentation Delay for an ASE.
             */
            int presentationDelayMin;
            /**
             * Maximum server supported Presentation Delay for an ASE.
             */
            int presentationDelayMax;
            /**
             * Preferred minimum Presentation Delay for an ASE.
             */
            int preferredPresentationDelayMin;
            /**
             * Preferred maximum Presentation Delay for an ASE.
             */
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
}
