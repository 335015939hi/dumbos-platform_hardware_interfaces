/*
 * Copyright (C) 2021 The Android Open Source Project
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
 * This is the result returned by the IAuthGraphCommon finalizeChannel() function.
 * @hide
 */
@VintfStability
parcelable CreateChannelResult {
    /**
     * The arc from per-boot-key to the channel key used to decrypt incoming messages. The public
     * signing key of the other party and the label 'incoming' are attached as the additional
     * authenticated data. Format is same as the output of AES-GCM encryption operation.
     */
    byte[] incomingChannelArc;

    /**
     * The arc from per-boot-key to the channel key used to encrypt outgoing messages. The public
     * signing key of the other party and the label 'incoming' are attached as the additional
     * authenticated data. Format is same as the output of AES-GCM encryption operation.
     */
    byte[] outgoingChannelArc;

    // TODO: add a third field with the MAC value on a fixed input.
}
