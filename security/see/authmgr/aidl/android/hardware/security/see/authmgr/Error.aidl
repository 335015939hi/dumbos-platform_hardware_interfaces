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

/**
 * AuthMgr error codes. Aidl will return these error codes as service specific errors in
 * EX_SERVICE_SPECIFIC.
 */
@VintfStability
@Backing(type="int")
enum Error {
    /* Success */
    OK = 0,
    /* Duplciated authenticated attempt with the same instance ID */
    INSTANCE_ALREADY_AUTHENTICATED = -1,
    /* Invalid DICE certificate chain of the AuthMgr FE */
    INVALID_DICE_CERT_CHAIN = -2,
    /* Invalid DICE leaf of the client */
    INVALID_DICE_LEAF = -3,
    /* Invalid DICE policy */
    INVALID_DICE_POLICY = -4,
    /* The DICE chain to policy matching failed */
    DICE_POLICY_MATCHING_FAILED = -5,
    /* Invalid signature */
    SIGNATURE_VERIFICATION_FAILED = -6,

}
