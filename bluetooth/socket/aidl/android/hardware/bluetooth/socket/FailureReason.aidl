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
 * Failure reason on IBluetoothSocketCallback.
 */
@VintfStability
@Backing(type="int")
enum FailureReason {
    /** Unspecified error occurs. */
    UNSPECIFIED = 0,

    /** Device does not support offload socket. */
    FEATURE_NOT_SUPPORTED = 1,

    /** A general/unknown failure occurred in application. */
    APP_INTERNAL_ERROR = 2,

    /** A general/unknown failure occurred in framework. */
    FRAMEWORK_INTERNAL_ERROR = 3,

    /** A general/unknown failure occurred in stack. */
    STACK_INTERNAL_ERROR = 4,

    /** Socket ID is not valid. */
    INVALID_SOCKET_ID = 5,

    /** Hub ID is not valid. */
    INVALID_HUB_ID = 6,

    /** Endpoint ID is not valid. */
    INVALID_ENDPOINT_ID = 7,

    /** ACL handle is not valid. */
    INVALID_ACL_HANDLE = 8,

    /** Channel information is not valid. */
    INVALID_CHANNEL_INFO = 9,

    /** Device does not have enough resources available to complete the operation. */
    OUT_OF_RESOURCES = 10,

    /** The operation failed due to an error in the vendoer-specific code. */
    VENDOR_SPECIFIC_ERROR = 0xFFFF,
}
