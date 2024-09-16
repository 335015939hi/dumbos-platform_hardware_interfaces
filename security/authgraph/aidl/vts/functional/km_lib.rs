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
    IAuthGraphKeyExchange::IAuthGraphKeyExchange, SessionIdSignature::SessionIdSignature,
    Identity::Identity, PlainPubKey::PlainPubKey, PubKey::PubKey,
};
use coset::{iana,};
use authgraph_boringssl as boring;
use authgraph_core::{arc, key, error::Error as AgError, keyexchange as ke, keymanagement as km};
use std::{cell::RefCell, rc::Rc};
use authgraph_boringssl::test_device::{self, AgDevice};
use authgraph_test_ids;
//use coset::CborSerializable;
pub mod km_sink;
pub mod km_source;

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
pub fn test_agkm_participant(source: bool) -> Result<AgkmTestContext, AgError> {
    let dev = boring::test_device::AgDevice::default_managed_device()?;
    let (self_id, src_id, sink_id, rotation_src_dentity, rotation_sink_identity) = get_ids(&dev);
    // The auth bound caps are used to test the source.
    let mut authbound_arc_caps = test_device::create_caps(None,
                             sink_id.policy.clone(),
                             arc::UIDPolicy::Single);
    // The credential arc used to test the source.
    let mut cred_arc = dev.get_cred_arc().expect("Default managed device should have credential arc");
    // Get the source and sink identity for this instance which will be
    // same as src_id and sink_id. But function invocations given below also returns
    // the private signing keys.
    let (src_pvt_key, cbor_identity) = authgraph_test_ids::create_identity(2, 2)
        .expect("src identity should be created");
    let src_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (sink_pvt_key, cbor_identity) = authgraph_test_ids::create_identity(3, 2)
        .expect("sink identity should be created");
    let sink_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    // If local test participant is source
    if source {
      // change the local test device identity to src
      dev.set_identity((src_pvt_key, src_identity.clone()), iana::Algorithm::EdDSA);
      // change the authbound arc caps to the self identity policy because
      // remote test device uses self identity.
      authbound_arc_caps = test_device::create_caps(None,
                                   self_id.policy.clone(),
                                   arc::UIDPolicy::Single);
      // change the credential arc to use local src identity.
      cred_arc = test_device::create_default_cred_arc(src_identity, &dev)?;
    }else{
      // Use the sink identity for local sink.
      dev.set_identity((sink_pvt_key, sink_identity), iana::Algorithm::EdDSA);
    }

    let agkm = km::AuthGraphParticipant::new(
                       boring::crypto_trait_impls(),
                       Rc::new(RefCell::new(dev)),
                   )?;
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
pub fn test_agke_participant(source: bool) -> Result<ke::AuthGraphParticipant, AgError> {
    // Get the source and sink identity for this instance which will be
    // same as src_id and sink_id. But function invocations given below also returns
    // the private signing keys.
    let dev = boring::test_device::AgDevice::default_managed_device()?;
    let (src_pvt_key, cbor_identity) = authgraph_test_ids::create_identity(2, 2)
        .expect("src identity should be created");
    let src_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (sink_pvt_key, cbor_identity) = authgraph_test_ids::create_identity(3, 2)
        .expect("sink identity should be created");
    let sink_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    // For source use the src identity else the sink identity
    if source {
      dev.set_identity((src_pvt_key, src_identity), iana::Algorithm::EdDSA);
    }else{
      dev.set_identity((sink_pvt_key, sink_identity), iana::Algorithm::EdDSA);
    }
    Ok(ke::AuthGraphParticipant::new(
        boring::crypto_trait_impls(),
        Rc::new(RefCell::new(dev)),
        ke::MAX_OPENED_SESSIONS,
    )?)
}

fn extract_plain_pub_key(pub_key: &Option<PubKey>) -> &PlainPubKey {
    match pub_key {
        Some(PubKey::PlainKey(pub_key)) => pub_key,
        Some(PubKey::SignedKey(_)) => panic!("expect unsigned public key"),
        None => panic!("expect pubKey to be populated"),
    }
}
/// Convert the vector to SessionIdSignature
pub fn vec_to_signature(data: &[u8]) -> SessionIdSignature {
    SessionIdSignature {
        signature: data.to_vec(),
    }
}

fn build_plain_pub_key(pub_key: &Option<Vec<u8>>) -> PubKey {
    PubKey::PlainKey(PlainPubKey {
        plainPubKey: pub_key.clone().unwrap(),
    })
}

fn vec_to_identity(data: &[u8]) -> Identity {
    Identity {
        identity: data.to_vec(),
    }
}