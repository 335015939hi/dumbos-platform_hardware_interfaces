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

package android.hardware.security.keymint;

import android.hardware.security.keymint.SecurityLevel;

/**
 * KeyMintHardwareInfo is the hardware information returned by calling KeyMint getHardwareInfo()
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true, Ord=true, PartialOrd=true, Hash=true)
parcelable KeyMintHardwareInfo {
    /**
     * The security level of the IKeyMintDevice implementation accessed through this aidl package.
     */
    SecurityLevel securityLevel = SecurityLevel.SOFTWARE;

    /**
     * keyMintAuthorName is the name of the author of the IKeyMintDevice implementation
     * (organization name, not individual).
     */
    @utf8InCpp String keyMintAuthorName;

    /**
     * keyMintName is the name of the IKeyMintDevice implementation.  This should provide enough
     * information to distinguish between KeyMint implementations from the same author.
     */
    @utf8InCpp String keyMintName;

    /**
     * Implementation version of the keymint implementation.  The version number structure is
     * implementation defined, and not necessarily globally meaningful.  The version is used to
     * distinguish between different versions of a given implementation.  Different releases should
     * have different numbers, and chronologically-later releases should generally have larger
     * numbers.
     */
    long versionNumber;

    /**
     * timestampTokenRequired is a boolean flag, which indicates that IKeyMintDevice instance will
     * expect a valid TimeStampToken with various operations. This will typically be required by
     * StrongBox implementations that generally don't have secure clock hardware to track time.
     */
    boolean timestampTokenRequired;

    /**
     * UUID provides a value that uniquely identifies a KeyMint implementation on a device.  No two
     * KeyMint implementations on the same device may return the same UUID, and the UUID returned by
     * a given implementation must not change between factory resets, even when the implementation
     * is upgraded to a new but compatible version.  The UUID may change upon factory reset, but is
     * not required to.
     */
    byte[16] uuid;
}
