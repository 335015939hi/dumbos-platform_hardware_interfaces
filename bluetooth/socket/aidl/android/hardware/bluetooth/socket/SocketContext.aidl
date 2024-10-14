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

import android.hardware.bluetooth.socket.ConnectionState;
import android.hardware.bluetooth.socket.DataPath;
import android.hardware.bluetooth.socket.ProtocolChannelInfo;

/**
 * Socket context.
 */
@VintfStability
parcelable SocketContext {
    /**
     * Socket connection ID assigned to the open connection on the BluetoothSocket.
     */
    long socketId;

    /**
     * Socket connection state.
     */
    ConnectionState state;

    /**
     * Descriptive socket name.
     */
    String name;

    /**
     * ACL handle for the socket connection.
     */
    int aclHandle;

    /**
     * Socket data offload path.
     */
    DataPath dataPath;

    /**
     * Channel information of the socket protocol.
     */
    ProtocolChannelInfo channelInfo;

    /**
     * The ID of the Hub to which the end point belongs for hardware offload data path.
     */
    long hubId;

    /**
     * The ID of the Hub endpoint for hardware offload data path.
     */
    long endpointId;
}
