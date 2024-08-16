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

package android.hardware.security.see.authmgr;

import android.hardware.security.see.authmgr.DicePolicy;
import android.hardware.security.see.authmgr.ExplicitKeyDiceCertChain;

/**
 * The information about the remote client passed to the trusted service by the AuthMgr BE, during
 * the connection handover in `ITrustedServicesCommonsConnect`.
 */
@VintfStability
@RustDerive(Clone=true, Eq=true, PartialEq=true)
parcelable ClientContext {
    /**
     * The full DICE chain of the client constructed with the DICE chain of the AuthMgr FE and the
     * DICE leaf certificate of the client.
     */
    ExplicitKeyDiceCertChain diceChain;
    /**
     * The full DICE policy of the client.
     */
    DicePolicy dicePolicy;
    /**
     * Indicates whether the client is persistent (i.e. whether the client survives factory reset).
     */
    boolean isPersistent;
}
