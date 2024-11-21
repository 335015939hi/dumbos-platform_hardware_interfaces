/*
 * Copyright 2024 The Android Open Source Project
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
package android.hardware.security.see.hdcp;

import android.hardware.drm.HdcpLevel;

/**
 * IHdcpAuthControl is used by the OEMCrypto Trusted Application to interact
 * with a HDCP Encryption Trusted Application in order to control the
 * HDCP Authentication Levels.
 */
//@VintfStability
interface IHdcpAuthControl {
    /**
     * Result returned from the getPendingHdcpLevelResult API.
     */
    parcelable PendingHdcpLevelResult {
        enum Status {
            /**
             * No pending HdcpLevel request
             */
            NONE,
            /**
             * a HdcpLevel request is pending, its level is provided in the
             * |level| attribute
             */
            PENDING,
        }
        Status status;
        @nullable HdcpLevel level;
    }

    /**
     * Return the currently negotiated and max supported HDCP levels.
     *
     * The current level is based on the display(s) the device is connected to.
     * If multiple HDCP-capable displays are simultaneously connected to
     * separate interfaces, this method returns the lowest negotiated HDCP level
     * of all interfaces.
     *
     * The maximum HDCP level is the highest level that can potentially be
     * negotiated. It is a constant for any device, i.e. it does not depend on
     * downstream receiving devices that could be connected. For example, if
     * the device has HDCP 1.x keys and is capable of negotiating HDCP 1.x, but
     * does not have HDCP 2.x keys, then the maximum HDCP capability would be
     * reported as 1.x. If multiple HDCP-capable interfaces are present, it
     * indicates the highest of the maximum HDCP levels of all interfaces.
     *
     * This method should only be used for informational purposes, not for
     * enforcing compliance with HDCP requirements. Trusted enforcement of HDCP
     * policies must be handled by the DRM system.
     *
     * @return HdcpLevels parcelable
     */
    HdcpLevels getHdcpLevels();

    /**
     * Attempts to set the device's HDCP auth level to |level|.
     *
     * @param level: desired HDCP level
     *
     * Return:
     *     a service specific error based on <code>HalErrorCode</code>,
     *     specifically:
     *       + BAD_PARAMETER: when HDCP_UNKNOWN is requested
     *       + UNSUPPORTED: when |level| is greater than the MaxLevel supported
     *       + BAD_STATE: when the HDCP's service currentLevel is HDCP_NO_OUTPUT
     *
     */
    trySetHdcpLevel(HdcpLevel level);

    /**
     * Retrieve the pending level currently being processed by the HDCP service.
     * The pending HDCP protection level might be higher than the level initially
     * requested. This can occur when multiple applications or services are
     * using HDCP concurrently, and a higher level is needed to satisfy
     * all requirements.
     *
     * Return:
     *      PendingHdcpLevelResult on success, which contains a status
     *      and an optional level; on error a service specific error based on
     *      <code>HalErrorCode</code> otherwise.
     *
     */
    HdcpLevel getPendingHdcpLevel();
}
