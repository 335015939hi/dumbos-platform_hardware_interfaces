/*
 * Copyright 2023 The Android Open Source Project
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

package android.hardware.bluetooth.audio;

import android.hardware.bluetooth.audio.CodecFormat;

/**
 * Codec Id as a triplet:
 * CodecFormat    Enumeration agnostic to transport
 * vendorId       16 bits - Assigned by BT Sig, when `codecId` set to vendor
 * vendorCodecId  16 bits - Assigned by the vendor
 *
 * The pair `vendorId` and `vendorCodecId` are set to `0` when `format` is not
 * set to `VENDOR`.
 */
@VintfStability
parcelable CodecId {
    CodecFormat format;
    int vendorId;
    int vendorCodecId;
}
