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
import android.hardware.security.authgraph.KeyWrappingKeyDeriveInfo;

/**
 * The enum type representing the artifacts required to recover a key wrapping key at two
 * instances:
 *     i. When recovering a key wrapping key that was derived via ECDH
 *    ii. When recovering a key wrapping key that was randomly created and protected with an
 *        auth key
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
union KeyWrappingKeyHandle {
    /**
     * Artifacts required to recover the key wrapping key for instance #1 above
     */
    KeyWrappingKeyDeriveInfo keyWrappingKeyDeriveArtifacts;
    /**
     * Artifacts required to recover the key wrapping key for instance #2 above
     */
    Arc lockedKeyWrappingKey;
}
