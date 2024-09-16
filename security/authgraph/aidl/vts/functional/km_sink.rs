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

//! VTS tests for sinks
use super::*;
use authgraph_core::{key, keymanagement as km};

/// Run AuthGraph tests against the provided sink, using a local test source implementation.
pub fn test(
    local_source: &mut km::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
    test_agkm_mainline(local_source, sink.clone());
}

/// Perform mainline AuthGraph key management with the provided sink and local implementation.
pub fn test_agkm_mainline(
    local_source: &mut km::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
  todo!()
}
