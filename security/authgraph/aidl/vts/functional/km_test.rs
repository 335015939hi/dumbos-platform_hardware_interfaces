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
    IAuthGraphKeyExchange::IAuthGraphKeyExchange,
};
use binder::StatusCode;

const AUTH_GRAPH_KM_NONSECURE: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyManagement/nonsecure";
const AUTH_GRAPH_KE_NONSECURE: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyExchange/nonsecure";

/// Retrieve the /nonsecure instance of AuthGraphKeyManagement, which supports both sink and source roles.
fn get_km_nonsecure() -> Option<binder::Strong<dyn IAuthGraphKeyManagement>> {
    match binder::get_interface(AUTH_GRAPH_KM_NONSECURE) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyManagement/nonsecure: {e:?}"),
    }
}

/// Retrieve the /nonsecure instance of AuthGraphKeyExchange, which supports both sink and source roles.
fn get_ke_nonsecure() -> Option<binder::Strong<dyn IAuthGraphKeyExchange>> {
    match binder::get_interface(AUTH_GRAPH_KE_NONSECURE) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyExchange/nonsecure: {e:?}"),
    }
}

/// Macro to require availability of a /nonsecure instance of AuthGraph.
///
/// Note that this macro triggers `return` if not found.
macro_rules! require_km_nonsecure {
    {} => {
        match get_km_nonsecure() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /nonsecure impl found");
                return;
            }
        }
    }
}
/// Macro to require availability of a /nonsecure instance of AuthGraph.
///
/// Note that this macro triggers `return` if not found.
macro_rules! require_ke_nonsecure {
    {} => {
        match get_ke_nonsecure() {
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
    // First perform key exchange
    let mut sink_ke = vts_km::test_agke_participant(false).expect("failed to create a local sink");
    let (src_out_arc, sink_in_arc) = vts_km::km_source::test_agke_mainline(&mut sink_ke, require_ke_nonsecure!());

    // Then perform key management
    let mut sink_ctx = vts_km::test_agkm_participant(false).expect("failed to create a local sink");
    // For default test device credential arc is always fixed.
    let cred_arc = sink_ctx.cred_arc.clone();
    vts_km::km_source::test_agkm_mainline(
      &mut sink_ctx,
      cred_arc,
      src_out_arc,
      sink_in_arc,
      require_km_nonsecure!());
}

#[test]
fn test_nonsecure_sink_mainline() {
    // First perform key exchange
    let mut source_ke = vts_km::test_agke_participant(true).expect("failed to create a local sink");
    let (src_out_arc, sink_in_arc) = vts_km::km_sink::test_agke_mainline(&mut source_ke, require_ke_nonsecure!());

    // Then perform key management
    let mut source_ctx = vts_km::test_agkm_participant(true).expect("failed to create a local source");
    // For default test device authbound arc capabilities are fixed.
    let caps = source_ctx.authbound_arc_caps.clone();
    vts_km::km_sink::test_agkm_mainline(
      &mut source_ctx,
      caps,
      src_out_arc,
      sink_in_arc,
      require_km_nonsecure!());
}