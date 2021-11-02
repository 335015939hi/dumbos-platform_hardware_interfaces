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

/**
 * This parcelable represents a sealed secret consisting of a cipher text, an AEAD tag, and an
 * initialization vector. It is used as return value of IDiceNode::seal.
 *
 * @hide
 */
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
@VintfStability
parcelable Sealed {
    /**
     * The sealed cipher text.
     */
    byte[] ciphertext;
    /**
     * The AEAD tag.
     */
    byte[] tag;
    /**
     * The initialization vector.
     */
    byte[] iv;
    /**
     * The salt used for key derivation.
     */
    byte[] salt;
}
