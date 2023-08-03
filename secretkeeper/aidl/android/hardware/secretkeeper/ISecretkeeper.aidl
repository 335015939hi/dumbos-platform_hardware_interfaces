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
     * This will be COSE_ENCRYPT
     *  structures using aad
     * operation caller_dice_chain, uid, key, value [key value will be encrypted]
     */
    void store(in byte[] payload);
    /**
     *  Read the secret
     */
    byte[] read(in byte[] payload);
}
