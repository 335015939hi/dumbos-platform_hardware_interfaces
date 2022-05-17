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
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package android.hardware.cas;
@Backing(type="int") @VintfStability
enum Status {
  OK = 0,
  ERROR_CAS_NO_LICENSE = 1,
  ERROR_CAS_LICENSE_EXPIRED = 2,
  ERROR_CAS_SESSION_NOT_OPENED = 3,
  ERROR_CAS_CANNOT_HANDLE = 4,
  ERROR_CAS_INVALID_STATE = 5,
  BAD_VALUE = 6,
  ERROR_CAS_NOT_PROVISIONED = 7,
  ERROR_CAS_RESOURCE_BUSY = 8,
  ERROR_CAS_INSUFFICIENT_OUTPUT_PROTECTION = 9,
  ERROR_CAS_TAMPER_DETECTED = 10,
  ERROR_CAS_DEVICE_REVOKED = 11,
  ERROR_CAS_DECRYPT_UNIT_NOT_INITIALIZED = 12,
  ERROR_CAS_DECRYPT = 13,
  ERROR_CAS_UNKNOWN = 14,
  ERROR_CAS_NEED_ACTIVATION = 15,
  ERROR_CAS_NEED_PAIRING = 16,
  ERROR_CAS_NO_CARD = 17,
  ERROR_CAS_CARD_MUTE = 18,
  ERROR_CAS_CARD_INVALID = 19,
  ERROR_CAS_BLACKOUT = 20,
  ERROR_CAS_REBOOTING = 21,
}
