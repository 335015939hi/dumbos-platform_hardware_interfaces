/*
 * Copyright (C) 2024 The Android Open Source Project
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

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.AuthKeyPackage;
import android.hardware.security.authgraph.AuthKeyPairPackage;
import android.hardware.security.authgraph.KeyWrappingKeyHandle;
import android.hardware.security.authgraph.KeyWrappingKeyPackage;

@VintfStability
interface IAuthGraphKeyManagement {
    /**
     * This method is invoked on a source TA, right after a user authenticates with the source TA
     * and the user's source key that is protected with the user's authentication factor is
     * unlocked.
     * Perform the following steps:
     *     1.
     *
     *
     */
    AuthKeyPackage[] createOrUnlockAuthKeys(in Arc sourceKey, in AuthKeyPackage[] authKeyPackages);

    AuthKeyPairPackage setupAuthKeyPair(in Arc channelKey, in Arc authKey);

    KeyWrappingKeyPackage createKeyWrappingKey(in Arc keyWrappingKeyInput);

    KeyWrappingKeyPackage recoverKeyWrappingKey(
            in KeyWrappingKeyHandle keyWrappingKeyHandle, in Arc authKey);

    /**
     * This method is invoked on a sink TA.
     * This is a helper method for the sink TA to decrypt the auth key (sent by the source TA) from
     * and re-encrypt with the sink TA's per-boot key.
     * @param channelKey - arc encrypting (with the sink'd per-boot key) the channel key setup with
     *                     source TA who creates the `authKey` arc
     * @param authKey - arc encrypting (with the channel key setup with the source TA) the auth
     *                  key created by the source TA
     *
     * @return Arc - arc encrypting the auth key with the sink TA's per-boot key
     */
    Arc unwrapAuthKey(in Arc channelKey, in Arc authKey);
}
