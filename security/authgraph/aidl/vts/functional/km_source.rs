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

//! VTS tests for sources
use super::*;
use authgraph_core::{key, keymanagement as km};

/// Run AuthGraph key management tests against the provided source, using a local test sink implementation.
pub fn test(
    local_sink: &mut km::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
    test_agkm_mainline(local_sink, source.clone());

}
/// Perform mainline AuthGraph key management with the provided source and local implementation.
pub fn test_agkm_mainline(
    local_sink: &mut km::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
  todo!()
}