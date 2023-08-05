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

package android.hardware.secretkeeper;

@VintfStability
@SensitiveData
interface ISecretkeeper {
    /**
     * Payload will be CBOR object containing the secret and additional data like
     * client uuid, policy. Secretkeeper will store the secret along with other data required for
     * authentication.
     */
    void store(in byte[] payload);
    /**
     *  Retrieve the client's secret. Payload will contain data to authenticate the client.
     */
    byte[] read(in byte[] payload);
}
