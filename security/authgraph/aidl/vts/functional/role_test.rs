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

use authgraph_vts_test as vts;
use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyExchange::IAuthGraphKeyExchange,
};

const AUTH_GRAPH_NONSECURE: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyExchange/nonsecure";

/// Retrieve the /nonsecure instance of AuthGraph, which supports both sink and source roles.
fn get_nonsecure() -> Option<binder::Strong<dyn IAuthGraphKeyExchange>> {
    binder::get_interface(AUTH_GRAPH_NONSECURE).ok()
}

#[test]
fn test_nonsecure_sink() {
    let sink = match get_nonsecure() {
        Some(v) => v,
        None => {
            eprintln!("Skipping test as no /nonsecure impl found");
            return;
        }
    };

    vts::sink::test(sink);
}

#[test]
fn test_nonsecure_source() {
    let source = match get_nonsecure() {
        Some(v) => v,
        None => {
            eprintln!("Skipping test as no /nonsecure impl found");
            return;
        }
    };

    vts::source::test(source);
}
