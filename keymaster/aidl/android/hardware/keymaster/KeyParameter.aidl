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


import android.hardware.identity.Algorithm;
import android.hardware.identity.BlockMode;
import android.hardware.identity.Digest;
import android.hardware.identity.EcCurve;
import android.hardware.identity.AuthenticationType;
import android.hardware.identity.KeyBlobUsageRequirements;
import android.hardware.identity.KeyDerivationFunction;
import android.hardware.identity.KeyOrigin;
import android.hardware.identity.KeyPurpose;
import android.hardware.identity.PaddingMode;
import android.hardware.identity.SecurityLevel;
import android.hardware.identity.IntegerParams;
import android.hardware.identity.Tag;


@VintfStability
parcelable KeyParameter {
    /**
     * Discriminates the union/blob field used.  The blob cannot be placed in the union, but only
     * one of "f" and "blob" may ever be used at a time.
     */
    Tag tag;

    union IntegerParams {
        /* Enum types */
        Algorithm algorithm;
        BlockMode blockMode;
        PaddingMode paddingMode;
        Digest digest;
        EcCurve ecCurve;
        KeyOrigin origin;
        KeyBlobUsageRequirements keyBlobUsageRequirements;
        KeyPurpose purpose;
        KeyDerivationFunction keyDerivationFunction;
        AuthenticationType hardwareAuthenticatorType;
        SecurityLevel hardwareType;

        /* Other types */
        bool boolValue;  // Always true, if a boolean tag is present.
        int integer;
        long longInteger;   // does long garantee 64???
        long dateTime;
    };

    int[] blob;
};
