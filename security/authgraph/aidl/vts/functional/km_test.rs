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

//! Tests of individual AuthGraph role (source or sink) functionality.

#![cfg(test)]

use authgraph_agkm_vts_test as vts_km;
use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyManagement::IAuthGraphKeyManagement,
};
use binder::StatusCode;

const AUTH_GRAPH_KEY_MNGMT_NONSECURE: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyManagement/nonsecure";

/// Retrieve the /nonsecure instance of AuthGraph, which supports both sink and source roles.
fn get_nonsecure() -> Option<binder::Strong<dyn IAuthGraphKeyManagement>> {
    match binder::get_interface(AUTH_GRAPH_KEY_MNGMT_NONSECURE) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraph/nonsecure: {e:?}"),
    }
}

/// Macro to require availability of a /nonsecure instance of AuthGraph.
///
/// Note that this macro triggers `return` if not found.
macro_rules! require_nonsecure {
    {} => {
        match get_nonsecure() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /nonsecure impl found");
                return;
            }
        }
    }
}

#[test]
fn test_nonsecure_source_mainline() {
    let mut sink_ctx = vts_km::test_agkm_participant().expect("failed to create a local sink");
    vts_km::source::test_agkm_mainline(&mut sink, require_nonsecure!());
}

#[test]
fn test_nonsecure_sink_mainline() {
    let mut source_ctx = vts_km::test_agkm_participant().expect("failed to create a local source");
    vts_km::sink::test_agkm_mainline(&mut source, require_nonsecure!());
}