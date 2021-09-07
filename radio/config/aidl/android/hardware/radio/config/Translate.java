/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.config;

public class Translate {
    static public android.hardware.radio.config.SimSlotStatus h2aTranslate(
            android.hardware.radio.config.V1_2.SimSlotStatus in) {
        android.hardware.radio.config.SimSlotStatus out =
                new android.hardware.radio.config.SimSlotStatus();
        out.cardState = in.base.cardState;
        out.slotState = in.base.slotState;
        out.atr = in.base.atr;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.base.logicalSlotId > 2147483647 || in.base.logicalSlotId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.base.logicalSlotId");
        }
        out.logicalSlotId = in.base.logicalSlotId;
        out.iccid = in.base.iccid;
        out.eid = in.eid;
        return out;
    }

    static public android.hardware.radio.config.PhoneCapability h2aTranslate(
            android.hardware.radio.config.V1_1.PhoneCapability in) {
        android.hardware.radio.config.PhoneCapability out =
                new android.hardware.radio.config.PhoneCapability();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.maxActiveData > 127 || in.maxActiveData < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.maxActiveData");
        }
        out.maxActiveData = in.maxActiveData;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.maxActiveInternetData > 127 || in.maxActiveInternetData < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.maxActiveInternetData");
        }
        out.maxActiveInternetData = in.maxActiveInternetData;
        out.isInternetLingeringSupported = in.isInternetLingeringSupported;
#error Arrays of NamedTypes are not currently not supported. Needs implementation for field: logicalModemList
        return out;
    }

    static public android.hardware.radio.config.ModemInfo h2aTranslate(
            android.hardware.radio.config.V1_1.ModemInfo in) {
        android.hardware.radio.config.ModemInfo out = new android.hardware.radio.config.ModemInfo();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.modemId > 127 || in.modemId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.modemId");
        }
        out.modemId = in.modemId;
        return out;
    }

    static public android.hardware.radio.config.ModemsConfig h2aTranslate(
            android.hardware.radio.config.V1_1.ModemsConfig in) {
        android.hardware.radio.config.ModemsConfig out =
                new android.hardware.radio.config.ModemsConfig();
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.numOfLiveModems > 127 || in.numOfLiveModems < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.numOfLiveModems");
        }
        out.numOfLiveModems = in.numOfLiveModems;
        return out;
    }

    static public android.hardware.radio.config.HalDeviceCapabilities h2aTranslate(
            android.hardware.radio.config.V1_3.HalDeviceCapabilities in) {
        android.hardware.radio.config.HalDeviceCapabilities out =
                new android.hardware.radio.config.HalDeviceCapabilities();
        out.modemReducedFeatureSet1 = in.modemReducedFeatureSet1;
        return out;
    }
}