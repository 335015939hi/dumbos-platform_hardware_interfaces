/*
 * Copyright (C) 2022 The Android Open Source Project
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

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.tv.cec;

public class Translate {
    static public android.hardware.tv.cec.HotplugEvent h2aTranslate(
            android.hardware.tv.cec.V1_0.HotplugEvent in) {
        android.hardware.tv.cec.HotplugEvent out = new android.hardware.tv.cec.HotplugEvent();
        out.connected = in.connected;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.portId > 2147483647 || in.portId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.portId");
        }
        out.portId = in.portId;
        return out;
    }

    static public android.hardware.tv.cec.HdmiPortInfo h2aTranslate(
            android.hardware.tv.cec.V1_0.HdmiPortInfo in) {
        android.hardware.tv.cec.HdmiPortInfo out = new android.hardware.tv.cec.HdmiPortInfo();
        out.type = in.type;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.portId > 2147483647 || in.portId < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.portId");
        }
        out.portId = in.portId;
        out.cecSupported = in.cecSupported;
        out.arcSupported = in.arcSupported;
        // FIXME This requires conversion between signed and unsigned. Change this if it doesn't
        // suit your needs.
        if (in.physicalAddress < 0) {
            throw new RuntimeException(
                    "Unsafe conversion between signed and unsigned scalars for field: in.physicalAddress");
        }
        out.physicalAddress = (char) in.physicalAddress;
        return out;
    }

    static public android.hardware.tv.cec.CecMessage h2aTranslate(
            android.hardware.tv.cec.V1_1.CecMessage in) {
        android.hardware.tv.cec.CecMessage out = new android.hardware.tv.cec.CecMessage();
        out.initiator = in.initiator;
        out.destination = in.destination;
        if (in.body != null) {
            out.body = new byte[in.body.size()];
            for (int i = 0; i < in.body.size(); i++) {
                // FIXME This requires conversion between signed and unsigned. Change this if it
                // doesn't suit your needs.
                if (in.body.get(i) > 127 || in.body.get(i) < 0) {
                    throw new RuntimeException(
                            "Unsafe conversion between signed and unsigned scalars for field: in.body.get(i)");
                }
                out.body[i] = in.body.get(i);
            }
        }
        return out;
    }
}