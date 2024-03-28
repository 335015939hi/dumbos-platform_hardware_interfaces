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
package android.hardware.security.see.authmgr;

parcelable SignedServerResponse {
    /**
     * A signed response from the server including:
     *
     * 1. The client challenge response. This is validated by the client.
     * 2. The server's SP-ID. The client should validate this against an entry in its device tree.
     * 3. A unique token for the client to use in all subsequent communication for this client's
     * boot.
     * 4. A server challenge for the client to complete mutual authentication.
     *
     * TODO: define in CDDL
     */
    byte[] signedResponse;
}
