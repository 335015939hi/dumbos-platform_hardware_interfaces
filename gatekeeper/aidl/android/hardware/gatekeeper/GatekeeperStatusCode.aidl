/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.hardware.gatekeeper;

/**
 * Gatekeeper response codes; success >= 0; error < 0
 */
@VintfStability
@Backing(type="int")
enum GatekeeperStatusCode {
    STATUS_REENROLL = 1, // success, but upper layers should re-enroll
                         // the verified password due to a version change
    STATUS_OK = 0, // operation is successful
    ERROR_GENERAL_FAILURE = -1, // operation failed
    ERROR_RETRY_TIMEOUT = -2, // operation should be retried after timeout
    ERROR_NOT_IMPLEMENTED = -3, // operation is not implemented
}
