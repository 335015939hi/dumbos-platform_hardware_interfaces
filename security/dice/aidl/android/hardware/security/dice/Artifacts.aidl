/*
 * Copyright 2021, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.security.dice;

import android.hardware.security.dice.Certificate;

/**
 * Represents one set of DICE_Artifacts required by a resident DICE node.
 *
 * @hide
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
parcelable Artifacts {
    /**
     * CDI_Seal.
     */
    byte[] cdiSeal;
    /**
     * CDI_Attest.
     */
    byte[] cdiAttest;
    /**
     * Chain of certificates constituting the attestation chain of the entity that the above
     * artifacts belong to.
     */
    Certificate[] attestationChain;
}
