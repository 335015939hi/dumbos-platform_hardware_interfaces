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

import android.hardware.bluetooth.socket.IBluetoothSocketCallback;
import android.hardware.bluetooth.socket.SocketContext;
import android.hardware.bluetooth.socket.SocketProperties;
import android.hardware.bluetooth.socket.Uuid;

/**
 * The interface for host stack to get socket properties, and notify socket connection state.
 */
@VintfStability
interface IBluetoothSocket {
    /**
     * API to initialize the socket interface and set the callbacks.
     *
     * @param callback Callbacks when incoming SocketEvent are received
     */
    void initialize(in IBluetoothSocketCallback callback);

    /**
     * API to get supported offload socket properties for each data path.
     *
     * @return an array of socket properties
     */
    @nullable SocketProperties[] getSocketProperties();

    /**
     * API to notify socket connection state to offload stack.
     *
     * @param context Socket context like socket id, channel info, data path, and end point info
     */
    void notifySocketConnectionStateChange(in SocketContext context);
}
