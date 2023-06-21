/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.security.authgraph;

/**
 * This is part of the return type: `KEResult` of the authenticated key exchange methods. This
 * includes the value of the session id computed by the two peers, that corresponds to the secure
 * channel established between them. Apart from the usage of session id by the two peers,
 * session id is also useful for testing purposes to verify (by a third party) that the key exchange
 * was successful.
 */
@VintfStability
parcelable SessionId {
    /**
     * TODO: define how this is computed and the data format.
     */
    byte[] encodedSessionId;
}
