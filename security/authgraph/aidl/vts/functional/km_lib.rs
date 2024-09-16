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
    IAuthGraphKeyManagement::IAuthGraphKeyManagement,Arc::Arc,Role::Role, Capability::Capability,
    Error::Error, IAuthGraphKeyExchange::IAuthGraphKeyExchange, Identity::Identity,
    PlainPubKey::PlainPubKey, PubKey::PubKey, SessionIdSignature::SessionIdSignature,
};

use authgraph_boringssl as boring;
use authgraph_core::{arc, key, error::Error as AgError, keyexchange as ke, keymanagement as km};
use std::{cell::RefCell, rc::Rc};
use authgraph_boringssl::test_device::{self, AgDevice};
use coset::CborSerializable;
pub mod km_sink;
pub mod km_source;
pub mod source;
pub mod sink;

#[allow(dead_code)]
pub struct AgkmTestContext{
    /// the agkm participant instance used as test TA
    pub agkm: km::AuthGraphParticipant,
    /// self identity of the participant
    pub self_id: key::Identity,
    /// source identity supported the participant
    pub src_id: key::Identity,
    /// sink identity supported the participant
    pub sink_id: key::Identity,
    /// source identity that will cause key rotation in the participant
    pub rotation_src_dentity: key::Identity,
    /// sink identity that will cause key rotation in the participant
    pub rotation_sink_identity: key::Identity,
    /// default credential arc of the participant
    pub cred_arc: Vec<u8>,
    /// default authbound sink specific authbound arc capabilities for the participant
    pub authbound_arc_caps: Vec<u8>,
}

/// Return an AuthGraphParticipant suitable for testing.
pub fn test_agkm_participant() -> Result<AgkmTestContext, AgError> {
    let dev = boring::test_device::AgDevice::default_managed_device()?;
    let (self_id, src_id, sink_id, rotation_src_dentity, rotation_sink_identity) = get_ids(&dev);
    let cred_arc = dev.get_cred_arc().expect("Default managed device should have credential arc");
    let agkm = km::AuthGraphParticipant::new(
                       boring::crypto_trait_impls(),
                       Rc::new(RefCell::new(dev)),
                   )?;
    let authbound_arc_caps = test_device::create_caps(None, sink_id.policy.clone(), arc::UIDPolicy::Single);
    Ok(AgkmTestContext{
      agkm, self_id, src_id, sink_id, rotation_src_dentity, rotation_sink_identity,cred_arc,authbound_arc_caps,
    })
}

fn get_ids(dev: &AgDevice) -> (key::Identity, key::Identity, key::Identity, key::Identity, key::Identity){
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

/// Return an AuthGraphParticipant suitable for testing.
pub fn test_agke_participant() -> Result<ke::AuthGraphParticipant, AgError> {
    Ok(ke::AuthGraphParticipant::new(
        boring::crypto_trait_impls(),
        Rc::new(RefCell::new(boring::test_device::AgDevice::default())),
        ke::MAX_OPENED_SESSIONS,
    )?)
}

fn build_plain_pub_key(pub_key: &Option<Vec<u8>>) -> PubKey {
    PubKey::PlainKey(PlainPubKey {
        plainPubKey: pub_key.clone().unwrap(),
    })
}

fn extract_plain_pub_key(pub_key: &Option<PubKey>) -> &PlainPubKey {
    match pub_key {
        Some(PubKey::PlainKey(pub_key)) => pub_key,
        Some(PubKey::SignedKey(_)) => panic!("expect unsigned public key"),
        None => panic!("expect pubKey to be populated"),
    }
}

fn vec_to_identity(data: &[u8]) -> Identity {
    Identity {
        identity: data.to_vec(),
    }
}

fn vec_to_signature(data: &[u8]) -> SessionIdSignature {
    SessionIdSignature {
        signature: data.to_vec(),
    }
}