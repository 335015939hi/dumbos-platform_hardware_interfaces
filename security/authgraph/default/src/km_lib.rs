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

//! Common functionality for non-secure/testing instance of AuthGraph.
extern crate alloc;
use authgraph_boringssl as boring;
use authgraph_core::{
    error, keyexchange, keymanagement,
    ta::{AuthGraphTa, Role}, ag_err, error::Error,
    key::{EcSignKey, Identity},
};
use authgraph_wire::{ErrorCode};
use authgraph_km_hal::channel::SerializedChannel;
use log::error;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::{mpsc, Mutex, Arc};
use boring::test_device::AgDevice;
use authgraph_test_ids;

/// Implementation of the AuthGraph TA that runs locally
pub struct LocalTa {
    channels: Mutex<Channels>,
}

struct Channels {
    in_tx: mpsc::Sender<Vec<u8>>,
    out_rx: mpsc::Receiver<Vec<u8>>,
}

/// Encapsulates LocalTa in Arc
pub struct SharedLocalTa {
  shared_local_ta: Arc<LocalTa>,
}

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Result<Self, error::Error> {
        // Create a pair of channels to communicate with the TA thread.
        let (in_tx, in_rx) = mpsc::channel();
        let (out_tx, out_rx) = mpsc::channel();
        // The TA code expects to run single threaded, so spawn a thread to run it in.
        std::thread::spawn(move || {
        let (self_pvt_sign_key,
         self_identity,
         src_identity,
         sink_identity,
         rotation_src_identity,
         rotation_sink_identity) = configure_default_device()
                                   .expect("failed to create identities");
        let dev = Rc::new(RefCell::new(AgDevice::default_managed_device(
                    self_pvt_sign_key,
                    self_identity,
                    src_identity,
                    sink_identity,
                    rotation_src_identity,
                    rotation_sink_identity)
                   .expect("failed to create default managed device")));
            let mut ta = AuthGraphTa::new_managed_ta(
                  keyexchange::AuthGraphParticipant::new(
                                  boring::crypto_trait_impls(),
                                  dev.clone(),
                                  keyexchange::MAX_OPENED_SESSIONS,
                  ).expect("failed to create AGKE participant"),
                  keymanagement::AuthGraphParticipant::new(
                                  boring::crypto_trait_impls(),
                                  dev.clone(),
                  ).expect("failed to create AGKM participant"),
                  Role::Both);
            // Loop forever processing request messages.
            loop {
                let req_data: Vec<u8> = match in_rx.recv() {
                    Ok(data) => data,
                    Err(_) => {
                        error!("local TA failed to receive request!");
                        break;
                    }
                };
                let rsp_data = ta.process(&req_data);
                match out_tx.send(rsp_data) {
                    Ok(_) => {}
                    Err(_) => {
                        error!("local TA failed to send out response");
                        break;
                    }
                }
            }
            error!("local TA terminating!");
        });
        Ok(Self {
            channels: Mutex::new(Channels { in_tx, out_rx }),
        })
    }
}

impl SerializedChannel for LocalTa {
    const MAX_SIZE: usize = usize::MAX;

    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        // Serialize across both request and response.
        let channels = self.channels.lock().unwrap();
        channels
            .in_tx
            .send(req_data.to_vec())
            .expect("failed to send in request");
        Ok(channels.out_rx.recv().expect("failed to receive response"))
    }
}

impl SharedLocalTa{
  pub fn new() -> Result<Self, error::Error> {
    Ok(Self {
      shared_local_ta : Arc::new(LocalTa::new()?),
    })
  }
  pub fn clone(&self) -> Self{
    Self{
      shared_local_ta: self.shared_local_ta.clone(),
    }
  }
}

impl SerializedChannel for SharedLocalTa {
  const MAX_SIZE: usize = usize::MAX;
  fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
    self.shared_local_ta.execute(req_data)
  }
}

// Configure the default managed device
// It configures 6 identities which are used for self identity and allowed sources
// and sinks. It configures default credential arc with hardcoded uid and
// source id. There are no limitations to this arc.
fn configure_default_device()
    -> Result<(EcSignKey, Identity, Identity, Identity, Identity, Identity), Error> {
    let (self_pvt_sign_key, cbor_identity) = authgraph_test_ids::create_identity(1, 2)
        .map_err(|e| ag_err!(InternalError, "creating self identity failed :{e:?}"))?;
    let self_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (_, cbor_identity) = authgraph_test_ids::create_identity(2, 2)
        .map_err(|e| ag_err!(InternalError, "creating src identity failed{e:?}"))?;
    let src_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (_, cbor_identity) = authgraph_test_ids::create_identity(3, 2)
        .map_err(|e| ag_err!(InternalError, "creating sink identity failed{e:?}"))?;
    let sink_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (_, cbor_identity) = authgraph_test_ids::create_identity(3, 3)
        .map_err(|e| ag_err!(InternalError, "creating rotation sink identity failed{e:?}"))?;
    let rotation_sink_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    let (_, cbor_identity) = authgraph_test_ids::create_identity(2, 3)
        .map_err(|e| ag_err!(InternalError, "creating rotation src identity failed{e:?}"))?;
    let rotation_src_identity = authgraph_test_ids::create_identity_with_policy(&cbor_identity);
    Ok((self_pvt_sign_key, self_identity, src_identity, sink_identity, rotation_src_identity, rotation_sink_identity))
}

///// create capability set
//pub fn create_caps(
//    source_ids: Option<Vec<DicePolicy>>,
//    sink_id: Option<DicePolicy>,
//    uid_policy: arc::UIDPolicy,
//) -> Vec<u8> {
//    let mint_perm = arc::MintingAllowed { source_ids, sink_id, uid_policy };
//    let caps = authgraph_core::keymanagement::CapabilitySet {
//        permissions: Some(arc::Permissions {
//            minting_allowed: Some(mint_perm.clone()),
//            ..Default::default()
//        }),
//        ..Default::default()
//    };
//    caps.to_cbor_value().unwrap().to_vec().unwrap()
//}