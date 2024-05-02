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

package android.hardware.security.gateweaver;

import android.hardware.security.authgraph.Arc;

/**
 * IGateWeaver provides unified way to protect user data gated by Lock Screen Knowledge Factor
 * (LSKF) with SE/TEE enforced rate limiting and secure deletion, on all devices.
 *
 * The secure storage of GateWeaver is structured as an arrat of slots follows:
 * TDDO:
 *
 * AuthGraph enabled GateWeaver provides:
 * 1) a unique cryptographic key for encrypting LSKF-gated user data,
 * 2) a proof of user authentication, and makes them available to any component which need them.
 * @hide
 */
@VintfStability
@SensitiveData
interface IGateWeaver {
    /**
     * Creation of a random cryptographic key (K_cred) associated wih the given slotID. K_cred is
     * protected with two layers of encryption. First, it is encrypted with the protector-key. The
     * output blob of the first encryption is then encrypted with a per-slot GW-main key.
     * Returns the GW blob which protects K_cred with these two layers of encryption
     */
    Arc create(in int slotID, in int[] schedule, in byte[] protector_key);
}
