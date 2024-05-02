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
 * The artifacts from re-enroll contains:
 *     i) the new GateWeaver blob created with the new protector key,
 *    ii) the arc encrypting the old K_cred with the GateWeaver's per-boot key
 *   iii) the arc encrypting the new K_cred with the GateWeaver's per-boot key
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable ReEnrollResponse {
    Arc newGWBlob;
    Arc authenticationWithExistingCredential;
    Arc authenticationWithNewCredential;
}
