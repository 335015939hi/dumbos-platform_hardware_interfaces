/*
 * Copyright (C) 2024 The Android Open Source Project
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

package android.hardware.security.gateweaver;
import android.hardware.security.authgraph.Arc;

/**
 * TimestampPackage holds the key shared between the GW and the secure clock services via the
 * protocol in IAuthGraphKeyExchange and the timestamp issued by the secure clock service.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable TimestampPackage {
    /**
     * The arc that encrypts the key shared between the GW and the secure clock service, with the
     * GW's per-boot per-boot key.
     */
    Arc sharedKey;
    /**
     * The arc that encrypts the timestamp with the key shared between the GW and the secure clock
     * service (i.e. the payload key in `sharedKey`)
     */
    Arc timestamp;
}
