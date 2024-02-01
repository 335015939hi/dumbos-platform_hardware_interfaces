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

/**
 * Key wrapping key is created by a sink to protect the user's secrets created at the sink. A unique
 * key wrapping key is created per each secret created by the user at the sink. The key wrapping
 * key can be unlocked only when the user's secret created at the corresponding source is unlocked.
 * This is the return type of both `createKeyWrappingKeyWithKeyExchange` and `createKeyWrappingKey`.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
union KeyWrappingKeyPackage {
    /**
     * A ephemeral arc encrypting the key wrapping key with the per-boot key of the sink.
     */
    Arc keyWrappingKey;
    /**
     * In `createKeyWrappingKeyWithKeyExchange`, this is a persistent arc authenticating the public
     * key of the ECDH key pair created to derive key wrapping key, with the sink's long term
     * encryption key.
     * In `createKeyWrappingKey`, and in `recoverKeyWrappingKeyWithKeyExchange` this is a persistent
     * arc encrypting the key wrapping key with the auth key.
     * In `recoverKeyWrappingKey`, this is optional.
     */
    @nullable Arc keyWrappingKeyHandle;
}
