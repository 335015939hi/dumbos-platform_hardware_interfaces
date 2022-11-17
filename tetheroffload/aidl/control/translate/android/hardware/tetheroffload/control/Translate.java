/**
 *
 * Copyright (C) 2022 The Android Open Source Project
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

// FIXME Remove this file if you don't need to translate types in this backend.

package android.hardware.tetheroffload.control;

public class Translate {
static public android.hardware.tetheroffload.control.IPv4AddrPortPair h2aTranslate(android.hardware.tetheroffload.control.V1_0.IPv4AddrPortPair in) {
    android.hardware.tetheroffload.control.IPv4AddrPortPair out = new android.hardware.tetheroffload.control.IPv4AddrPortPair();
    out.addr = in.addr;
    // FIXME This requires conversion between signed and unsigned. Change this if it doesn't suit your needs.
    if (in.port < 0) {
        throw new RuntimeException("Unsafe conversion between signed and unsigned scalars for field: in.port");
    }
<<<<<<< HEAD
    out.port = (char) in.port;
=======
    out.port = (int) in.port;
>>>>>>> e600a3416... Add autoconverted AIDL HALs for tetheroffload.
    return out;
}

static public android.hardware.tetheroffload.control.NatTimeoutUpdate h2aTranslate(android.hardware.tetheroffload.control.V1_0.NatTimeoutUpdate in) {
    android.hardware.tetheroffload.control.NatTimeoutUpdate out = new android.hardware.tetheroffload.control.NatTimeoutUpdate();
    out.src = h2aTranslate(in.src);
    out.dst = h2aTranslate(in.dst);
    out.proto = in.proto;
    return out;
}

}