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
 * Source key is an unique key created by a source. This is the root AuthGraph secret for a
 * particular user and it is protected with the user's credential key. `SourceKey` type holds two
 * acrs:
 *     i. persistent - the source key is encrypted with credential key of the user.
 *    ii. ephemeral - the source key is encrypted with the per-boot key of the source.
 * This package of information is used by the source.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable SourceKey {
    @nullable Arc persistent;
    @nullable Arc ephemeral;
}
