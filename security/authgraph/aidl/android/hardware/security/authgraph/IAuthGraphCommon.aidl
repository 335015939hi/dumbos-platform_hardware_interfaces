/*
 * Copyright (C) 2020 The Android Open Source Project
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

import android.hardware.security.authgraph.CreateChannelResult.aidl;
import android.hardware.security.authgraph.InitChannelResult.aidl;
import android.hardware.security.authgraph.VerifyChannelResult.aidl;

@VintfStability
interface IKeyMintDevice {
    InitChannelResult initChannel();

    CreateChannelResult createChannel(
            in byte[] pubKeyOfOtherParty, in @nullable InitChannelResult initChannelResult);

    VerifyChannelResult verifyChannel(in byte[] signatureOfOtherParty,
            in @nullable VerifyChannelResult verifyChannelResult);
}
