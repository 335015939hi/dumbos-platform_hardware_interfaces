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
 * Capabilities for LE L2CAP COC that the offload stack supports.
 *
 * Note that for the offload socket host stack will give zero LE L2CAP COC credit to peer device
 * during connection setup. It means offload stack should send initial credits to peer device
 * through L2CAP signaling command L2CAP_FLOW_CONTROL_CREDIT_IND when IBluetoothSocket.open() is
 * successful.
 */
@VintfStability
parcelable LeCocCapabilities {
    /**
     * Maximum number of LE COC sockets supported. If not supported, the value should be zero.
     */
    int numOfSocketSupported;

    /**
     * Local Maximum Transmission Unit for LE COC specifying the maximum SDU size (in bytes) that
     * the local L2CAP layer can receive. The MTU size should be in range 23 to 65535.
     */
    int mtu;
}
