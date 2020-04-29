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

package android.hardware.keymaster;


import android.hardware.keymaster.Algorithm;
import android.hardware.keymaster.BlockMode;
import android.hardware.keymaster.Digest;
import android.hardware.keymaster.EcCurve;
import android.hardware.keymaster.HardwareAuthenticatorType;
import android.hardware.keymaster.KeyBlobUsageRequirements;
import android.hardware.keymaster.KeyDerivationFunction;
import android.hardware.keymaster.KeyOrigin;
import android.hardware.keymaster.KeyPurpose;
import android.hardware.keymaster.PaddingMode;
import android.hardware.keymaster.SecurityLevel;
import android.hardware.keymaster.Tag;


/* union is currently not supported in aidl.  For now, we will just have the
 * Tags, and bool, int, long, int[], and we will cast to the appropate types
 * base on the Tag value.
 *
 * Please don't delete the commented out codes.
 */
@VintfStability
parcelable KeyParameter {
    /**
     * Discriminates the union/blob field used.  The blob cannot be placed in the union, but only
     * one of "f" and "blob" may ever be used at a time.
     */
    Tag tag;

/*
     union IntegerParams {
        // Enum types
        Algorithm algorithm;
        BlockMode blockMode;
        PaddingMode paddingMode;
        Digest digest;
        EcCurve ecCurve;
        KeyOrigin origin;
        KeyBlobUsageRequirements keyBlobUsageRequirements;
        KeyPurpose purpose;
        KeyDerivationFunction keyDerivationFunction;
        HardwareAuthenticatorType hardwareAuthenticatorType;
        SecurityLevel hardwareType;
*/
        /* Other types */
        boolean boolValue;  // Always true, if a boolean tag is present.   ??? this comment is invalid per parsing code in system/km
        int integer;
        long longInteger;
        long dateTime;     // these 4 variables are for referencing and casting purposes, so there is no need for dateTime it seems ???
   // };

    byte[] blob;
}
