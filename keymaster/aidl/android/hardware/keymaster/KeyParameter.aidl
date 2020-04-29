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


/**
 * Identifies the key authorization parameters to be used with keymaster.  This is usually
 * provided as an array of KeyParameters to IKeymasterDevice or Operation.
 *
 * TODO(seleneh): Union is currently not supported in aidl.  For now, we will just have the
 * Tags, and bool, int, long, int[], and we will cast to the appropate types base on the
 * Tag value.  We need to update this defination to distingish Algorithm, BlockMode,
 * PaddingMode, KeyOrigin...etc when union support is added to aidl.
 */
@VintfStability
parcelable KeyParameter {
    /**
     * Identify what type of key parameter this parcelable actually holds, and based on the type
     * of tag is int, long, bool, or byte[], one of the fields below will be referenced.
     */
    Tag tag;

    boolean boolValue;
    int integer;
    long longInteger;
    long dateTime;
    byte[] blob;
}
