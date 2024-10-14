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

package android.hardware.bluetooth.socket;

/**
 * LE L2CAP COC channel information
 */
@VintfStability
parcelable LeCocChannelInfo {
    /**
     * L2cap local channel ID.
     */
    int localCid;

    /**
     * L2cap remote channel ID.
     */
    int remoteCid;

    /**
     * PSM for L2CAP LE CoC.
     */
    int psm;

    /**
     * Local Maximum Transmission Unit for LE COC specifying the maximum SDU size in bytes that the
     * local L2CAP layer can receive.
     */
    int localMtu;

    /**
     * Remote Maximum Transmission Unit for LE COC specifying the maximum SDU size in bytes that the
     * remote L2CAP layer can receive.
     */
    int remoteMtu;

    /**
     * Local Maximum PDU payload Size in bytes that the local L2CAP layer can receive.
     */
    int localMps;

    /**
     * Remote Maximum PDU payload Size in bytes that the remote L2CAP layer can receive.
     */
    int remoteMps;

    /**
     * Protocol initial credits at Rx path.
     *
     * For the offload socket, host stack will give zero LE L2CAP COC credit to peer device during
     * connection setup. So, initialRxCredits would be zero. It means offload stack should send
     * initial credits to peer device through L2CAP signaling command L2CAP_FLOW_CONTROL_CREDIT_IND
     * when IBluetoothSocket.opened() is successful.
     */
    int initialRxCredits;

    /**
     * Protocol initial credits at Tx path.
     */
    int initialTxCredits;
}
