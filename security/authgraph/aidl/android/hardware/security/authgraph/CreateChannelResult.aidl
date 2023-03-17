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
 * This is the result returned by the IAuthGraphCommon createChannel() function.
 * @hide
 */
@VintfStability
parcelable CreateChannelResult {
    /**
     * The arc from per-boot-key to the channel key derived from the shared secret.
     * The public signing key of the other party is attached as the additional authenticated data.
     */
    byte[] channelArc;

    /**
     * The output of HMAC computation on a fixed value: "CHANNEL_ESTABLISHMENT_COMPLETED", using the
     * HMAC key derived from the shared secret. This is just to verify that the two parties have
     * performed channel establishment successfully.
     */
    byte[] macOutput;
}
