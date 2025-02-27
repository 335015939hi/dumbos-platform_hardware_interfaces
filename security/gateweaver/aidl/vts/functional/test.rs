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

const AUTH_GRAPH_KM_GATEWEAVER: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyManagement/gateweaver";
const AUTH_GRAPH_KE_GATEWEAVER: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyExchange/gateweaver";

/// Retrieve the /nonsecure instance of AuthGraphKeyManagement, which supports both sink and source roles.
fn get_km_gw() -> Option<binder::Strong<dyn IAuthGraphKeyManagement>> {
    match binder::get_interface(AUTH_GRAPH_KM_GATEWEAVER) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyManagement/gateweaver: {e:?}"),
    }
}

/// Retrieve the /nonsecure instance of AuthGraphKeyExchange, which supports both sink and source roles.
fn get_ke_gw() -> Option<binder::Strong<dyn IAuthGraphKeyExchange>> {
    match binder::get_interface(AUTH_GRAPH_KE_GATEWEAVER) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyExchange/gateweaver: {e:?}"),
    }
}

/// Macro to require availability of km service
macro_rules! require_km {
    {} => {
        match get_km_gw() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /gateweaver impl found");
                return;
            }
        }
    }
}

/// Macro to require availability of ke service
macro_rules! require_ke {
    {} => {
        match get_ke_gw() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /gateweaver impl found");
                return;
            }
        }
    }
}

#[test]
fn test_ag_gw_mainline() {
    // First perform key exchange
    let mut sink_ke = vts_km::test_agke_participant(false).expect("failed to create a local sink");

    //TODO replace the following with gateweaver ta
    let (src_out_arc, sink_in_arc) = vts_km::km_source::test_agke_mainline(&mut sink_ke, require_ke!());

    // Then perform key management
    let mut sink_ctx = vts_km::test_agkm_participant(false).expect("failed to create a local sink");
    //TODO replace the following with gateweaver ta
    // For default test device credential arc is always fixed.
    let cred_arc = sink_ctx.cred_arc.clone();
    vts_km::km_source::test_agkm_mainline(
      &mut sink_ctx,
      cred_arc,
      src_out_arc,
      sink_in_arc,
      require_km!());
}
