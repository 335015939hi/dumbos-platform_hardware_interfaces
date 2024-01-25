/*
 * Copyright (C) 2023 The Android Open Source Project
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

//! Test helper functions.

pub mod diags;
pub mod dice_sample;

// Constants for DICE map keys from the Open Profile for DICE; all bstr unless indicated.

/// Map key for code hash.
pub const CODE_HASH: i64 = -4670545;
/// Map key for code descriptor.
pub const CODE_DESC: i64 = -4670546;
/// Map key for config hash.
pub const CONFIG_HASH: i64 = -4670547;
/// Map key for config descriptor.
pub const CONFIG_DESC: i64 = -4670548;
/// Map key for authority hash.
pub const AUTHORITY_HASH: i64 = -4670549;
/// Map key for authority descriptor.
pub const AUTHORITY_DESCRIPTOR: i64 = -4670550;
/// Map key for mode.
pub const MODE: i64 = -4670551;
/// Map key for subject public key.
pub const SUBJECT_PUBLIC_KEY: i64 = -4670552;
/// Map key for key usage.
pub const KEY_USAGE: i64 = -4670553;
/// Map key for profile name (tstr).
pub const PROFILE_NAME: i64 = -4670554;

// Constants for DICE map keys in RKP.

/// Map key for component name (tstr).
pub const COMPONENT_NAME: i64 = -70002;
/// Map key for component version (int/tstr).
pub const COMPONENT_VERSION: i64 = -70003;
/// Map key for resettable (null).
pub const RESETTABLE: i64 = -70004;
/// Map key for security version (uint).
pub const SECURITY_VERSION: i64 = -70005;
/// Map key for RKP VM marker (null).
pub const RKP_VM: i64 = -70006;

// Additional constants for DICE map keys for RKP VMs.

/// Map key for payload config filename (tstr).
pub const PAYLOAD_CONFIG_FILENAME: i64 = -71000;
/// Map key for payload config (PayloadConfig).
pub const PAYLOAD_CONFIG: i64 = -71001;
/// Map key for subcomponent descriptor (array of SubcomponentDescriptor).
pub const CONFIG_DESC_SUB_COMPONENTS: i64 = -71002;
