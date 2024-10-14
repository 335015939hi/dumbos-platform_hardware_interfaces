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

import android.hardware.bluetooth.socket.FailureReason;
import android.hardware.bluetooth.socket.RequestReason;

/**
 * The interface from the Bluetooth offload socket to the host stack.
 */
@VintfStability
interface IBluetoothSocketCallback {
    /**
     * Invoked when IBluetoothSocket.open() is successful.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     */
    void onOpened(long socketId);

    /**
     * Invoked when IBluetoothSocket.open() fails.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     * @param reason Reason for failure while opening socket
     * @param vendorStatusCode Optional vendor-defined status code. The value 0 is reserved to
     *     indicate that a vendor status code was not provided or is not relevant. All other values
     *     have a vendor-defined meaning.
     */
    void onOpenFailed(long socketId, FailureReason reason, int vendorStatusCode);

    /**
     * Invoked when IBluetoothSocket.close() is successful.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     */
    void onClosed(long socketId);

    /**
     * Invoked when IBluetoothSocket.close() fails.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     * @param reason Reason for failure while closing socket
     * @param vendorStatusCode Optional vendor-defined status code. The value 0 is reserved to
     *     indicate that a vendor status code was not provided or is not relevant. All other values
     *     have a vendor-defined meaning.
     */
    void onCloseFailed(long socketId, FailureReason reason, int vendorStatusCode);

    /**
     * Invoked when offload stack requests host stack to close the socket.
     *
     * @param socketId Identifier assigned to the socket by the host stack
     * @param reason Reason for requesting socket close
     * @param vendorStatusCode Optional vendor-defined status code. The value 0 is reserved to
     *     indicate that a vendor status code was not provided or is not relevant. All other values
     *     have a vendor-defined meaning.
     */
    void onCloseRequest(long socketId, RequestReason reason, int vendorStatusCode);

    /**
     * Invoked when offload stack notifies the host stack that it has restarted.
     *
     * @param vendorStatusCode Optional vendor-defined status code. The value 0 is reserved to
     *     indicate that a vendor status code was not provided or is not relevant. All other values
     *     have a vendor-defined meaning.
     */
    void onRestarted(int vendorStatusCode);
}
