<<<<<<< HEAD   (e5b74f Merge empty history for sparse-11111303-L90100030000647828)
||||||| BASE
/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.sim;
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum PersoSubstate {
  UNKNOWN = 0,
  IN_PROGRESS = 1,
  READY = 2,
  SIM_NETWORK = 3,
  SIM_NETWORK_SUBSET = 4,
  SIM_CORPORATE = 5,
  SIM_SERVICE_PROVIDER = 6,
  SIM_SIM = 7,
  SIM_NETWORK_PUK = 8,
  SIM_NETWORK_SUBSET_PUK = 9,
  SIM_CORPORATE_PUK = 10,
  SIM_SERVICE_PROVIDER_PUK = 11,
  SIM_SIM_PUK = 12,
  RUIM_NETWORK1 = 13,
  RUIM_NETWORK2 = 14,
  RUIM_HRPD = 15,
  RUIM_CORPORATE = 16,
  RUIM_SERVICE_PROVIDER = 17,
  RUIM_RUIM = 18,
  RUIM_NETWORK1_PUK = 19,
  RUIM_NETWORK2_PUK = 20,
  RUIM_HRPD_PUK = 21,
  RUIM_CORPORATE_PUK = 22,
  RUIM_SERVICE_PROVIDER_PUK = 23,
  RUIM_RUIM_PUK = 24,
  SIM_SPN = 25,
  SIM_SPN_PUK = 26,
  SIM_SP_EHPLMN = 27,
  SIM_SP_EHPLMN_PUK = 28,
  SIM_ICCID = 29,
  SIM_ICCID_PUK = 30,
  SIM_IMPI = 31,
  SIM_IMPI_PUK = 32,
  SIM_NS_SP = 33,
  SIM_NS_SP_PUK = 34,
}
=======
/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.radio.sim;
@Backing(type="int") @JavaDerive(toString=true) @VintfStability
enum PersoSubstate {
  UNKNOWN,
  IN_PROGRESS,
  READY,
  SIM_NETWORK,
  SIM_NETWORK_SUBSET,
  SIM_CORPORATE,
  SIM_SERVICE_PROVIDER,
  SIM_SIM,
  SIM_NETWORK_PUK,
  SIM_NETWORK_SUBSET_PUK,
  SIM_CORPORATE_PUK,
  SIM_SERVICE_PROVIDER_PUK,
  SIM_SIM_PUK,
  RUIM_NETWORK1,
  RUIM_NETWORK2,
  RUIM_HRPD,
  RUIM_CORPORATE,
  RUIM_SERVICE_PROVIDER,
  RUIM_RUIM,
  RUIM_NETWORK1_PUK,
  RUIM_NETWORK2_PUK,
  RUIM_HRPD_PUK,
  RUIM_CORPORATE_PUK,
  RUIM_SERVICE_PROVIDER_PUK,
  RUIM_RUIM_PUK,
  SIM_SPN,
  SIM_SPN_PUK,
  SIM_SP_EHPLMN,
  SIM_SP_EHPLMN_PUK,
  SIM_ICCID,
  SIM_ICCID_PUK,
  SIM_IMPI,
  SIM_IMPI_PUK,
  SIM_NS_SP,
  SIM_NS_SP_PUK,
}
>>>>>>> BRANCH (ec4e12 Merge cherrypicks of ['android-review.googlesource.com/34619)
