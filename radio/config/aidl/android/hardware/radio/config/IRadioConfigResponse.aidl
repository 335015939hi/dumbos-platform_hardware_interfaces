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

package android.hardware.radio.config;

import android.hardware.radio.config.ModemsConfig;
import android.hardware.radio.config.PhoneCapability;
import android.hardware.radio.config.SimSlotStatus;

/**
 * Interface declaring response functions to solicited radio config requests.
 */
@VintfStability
interface IRadioConfigResponse {
    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param modemReducedFeatureSet1 True indicates that the modem does NOT support the following
     *        features.
     *        - Providing either
     *          android.hardware.radio::LinkCapacityEstimate:secondaryDownlinkCapacityKbps
     *          or android.hardware.radio::LinkCapacityEstimate:secondaryUplinkCapacityKbps
     *          when given from
     *          android.hardware.radio::RadioIndication:currentLinkCapacityEstimate
     *        - Calling android.hardware.radio::IRadio.setNrDualConnectivityState
     *          or querying android.hardware.radio::IRadio.isNrDualConnectivityEnabled
     *        - Requesting android.hardware.radio::IRadio.setDataThrottling()
     *        - Providing android.hardware.radio::SlicingConfig through
     *          android.hardware.radio::getSlicingConfig()
     *        - Providing android.hardware.radio::PhysicalChannelConfig through
     *          android.hardware.radio::IRadioIndication.currentPhysicalChannelConfigs_1_6()
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    oneway void getHalDeviceCapabilitiesResponse(
            in android.hardware.radio.RadioResponseInfo info, in boolean modemReducedFeatureSet1);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param modemsConfig <ModemsConfig> it defines all the modems' configurations
     *        at this time, only the number of live modems
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     */
    oneway void getModemsConfigResponse(
            in android.hardware.radio.RadioResponseInfo info, in ModemsConfig modemsConfig);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param phoneCapability <::PhoneCapability> it defines modem's capability for example
     *        how many logical modems it has, how many data connections it supports.
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     */
    oneway void getPhoneCapabilityResponse(
            in android.hardware.radio.RadioResponseInfo info, in PhoneCapability phoneCapability);

    /**
     * @param info Response info struct containing response type, serial no. and error
     * @param slotStatus Sim slot struct containing all the physical SIM slots info with size
     *        equal to the number of physical slots on the device
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     */
    oneway void getSimSlotsStatusResponse(
            in android.hardware.radio.RadioResponseInfo info, in SimSlotStatus[] slotStatus);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:REQUEST_NOT_SUPPORTED
     *   RadioError:INVALID_ARGUMENTS
     */
    oneway void setModemsConfigResponse(in android.hardware.radio.RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:INTERNAL_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    oneway void setPreferredDataModemResponse(in android.hardware.radio.RadioResponseInfo info);

    /**
     * @param info Response info struct containing response type, serial no. and error
     *
     * Valid errors returned:
     *   RadioError:NONE
     *   RadioError:RADIO_NOT_AVAILABLE
     *   RadioError:NO_MEMORY
     *   RadioError:INTERNAL_ERR
     *   RadioError:MODEM_ERR
     *   RadioError:INVALID_ARGUMENTS
     */
    oneway void setSimSlotsMappingResponse(in android.hardware.radio.RadioResponseInfo info);
}
