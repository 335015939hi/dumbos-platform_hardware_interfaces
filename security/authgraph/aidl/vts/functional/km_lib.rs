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

//! VTS test library for AuthGraph functionality.
//!
//! This test code is bundled as a library, not as `[cfg(test)]`, to allow it to be
//! re-used inside the (Rust) VTS tests of components that use AuthGraph.

use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    Error::Error, IAuthGraphKeyManagement::IAuthGraphKeyManagement, Identity::Identity,
    PlainPubKey::PlainPubKey, PubKey::PubKey, SessionIdSignature::SessionIdSignature,
};
use authgraph_boringssl as boring;
use authgraph_core::{error::Error as AgError, keymanagement as km};
use coset::CborSerializable;
use std::{cell::RefCell, rc::Rc};

pub mod km_sink;
pub mod km_source;
pub struct AgkmTestContext{
    agkm: km::AuthGraphParticipant,
    self_id: Identity,
    src_id: Identity,
    sink_id: Identity,
    rotation_src_dentity: Identity,
    rotation_sink_identity: Identity,
    cred_arc: Vec<u8>,
};

/// Return an AuthGraphParticipant suitable for testing.
pub fn test_agkm_participant() -> Result<AgkmTestContext, AgError> {
    let dev = boring::test_device::AgDevice::default_managed_device()?;
    let (self_id, src_id, sink_id, rotation_src_dentity, rotation_sink_identity) = get_ids(&dev);
    let cred_arc = dev.get_cred_arc().expect("Default managed device should have credential arc");
    let agkm = km::AuthGraphParticipant::new(
                       boring::crypto_trait_impls(),
                       Rc::new(RefCell::new(dev)),
                   )?;
    Ok(AgkmTestContext{
      agkm, self_id, src_id, sink_id, rotation_src_dentity, rotation_sink_identity,cred_src,
    })
}

fn get_ids(dev: &AgDevice) -> (Identity, Identity, Identity, Identity, Identity){
  let src_ids = dev.get_allowed_src_ids();
  let sink_ids = dev.get_allowed_sink_ids();
  assert!((src_ids.is_some() && sink_ids.is_some()), "allowed identities is missing from default device");
  let mut src_ids = src_ids.unwrap();
  let mut sink_ids = sink_ids.unwrap();
  // default device must have 3 sink ids and 3 src ids.
  assert!((src_ids.len() == 3 && sink_ids.len() == 3), "Wrong number of identities");
  src_ids.reverse();
  sink_ids.reverse();
  let _ = sink_ids.pop();
  (src_ids.pop().unwrap(), src_ids.pop().unwrap(), sink_ids.pop().unwrap(), src_ids.pop().unwrap(), sink_ids.pop().unwrap())
}