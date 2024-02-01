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

/**
 * Auth key is an unique key created by a source in order for a particular sink to protect the
 * sink's secret key material with a key held by the source. `AuthKey` type holds two acrs:
 *     i. persistent - the auth key is encrypted with a persistent key known to the source. The
 *                     arc is persistent as well.
 *    ii. ephemeral - the auth key is encrypted with the key shared between the source and the sink
 *                    during a particular boot. The arc is ephemeral as well.
 * This package of information is used by the source.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable AuthKey {
    @nullable Arc persistent;
    @nullable Arc ephemeral;
}
