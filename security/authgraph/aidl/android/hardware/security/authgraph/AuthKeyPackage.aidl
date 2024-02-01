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

package android.hardware.security.authgraph;

import android.hardware.security.authgraph.Arc;
import android.hardware.security.authgraph.AuthKey;

/**
 * AuthKeyPackage holds the mapping between the key shared between a source and a sink via the
 * protocol in IAuthGraphKeyExchange API and the unique AuthKey created by the source for the sink.
 * This package of information is used by the source.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable AuthKeyPackage {
    /**
     * The arc that encrypts the key shared between the source and the sink, with the source's
     * per-boot key.
     */
    Arc sharedKey;
    /**
     * The auth key created by the source for the sink. See AuthKey.aidl
     */
    @nullable AuthKey authKey;
}
