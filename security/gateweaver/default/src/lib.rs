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

//! Non-secure implementation of a local Secretkeeper TA.
extern crate alloc;
use authgraph_boringssl as boring;
use authgraph_core::{keyexchange, keymanagement};
use authgraph_core::ta::{AuthGraphTa, Role};
use authgraph_km_hal::channel::SerializedChannel;
use log::error;
use std::cell::RefCell;
use std::rc::Rc;
use std::sync::mpsc;
use std::sync::{Arc, Mutex};
use std::array;
use gateweaver::{CredentialStore, ta::GateWeaverTa, gw_err, wire::{self, Error, ErrorCode},};
use authgraph_core::key::{EcSignKey, Identity, AesKey, AES_256_KEY_LEN};
//use authgraph_test_ids as test_ids;

/// Implementation of the Local Gateweaver TA that runs locally in-process (and
/// which is therefore not secure).
pub struct LocalTa {
    in_tx: mpsc::Sender<Vec<u8>>,
    out_rx: mpsc::Receiver<Vec<u8>>,
}

/// Prefix byte for messages intended for the AuthGraph TA.
const AG_MESSAGE_PREFIX: u8 = 0x00;
/// Prefix byte for messages intended for the Gateweaver TA.
const GW_MESSAGE_PREFIX: u8 = 0x01;
/// maximum number of slots in the store
const GW_MAX_SLOTS: i32 = 32;

impl LocalTa {
    /// Create a new instance.
    pub fn new() -> Self {
        // Create a pair of channels to communicate with the TA thread.
        let (in_tx, in_rx) = mpsc::channel();
        let (out_tx, out_rx) = mpsc::channel();

        // The TA code expects to run single threaded, so spawn a thread to run it in.
        std::thread::spawn(move || {
            let storage_impl = Box::new(crate::InMemoryStore::new());
            let (identity_sign_key, identity) =
                crate::get_id().expect("Failed to read test ids");
            // create gatewear ta
            let gw_ta = Rc::new(RefCell::new(
                GateWeaverTa::new(
                  boring::crypto_trait_impls(),
                  identity_sign_key,
                  identity,
                  storage_impl,
              )
              .expect("Failed to create local Secretkeeper TA"),
            ));

            // create authgraph ta
            let mut ag_ta = AuthGraphTa::new_managed_ta(
                  keyexchange::AuthGraphParticipant::new(
                                  boring::crypto_trait_impls(),
                                  gw_ta.clone(),
                                  keyexchange::MAX_OPENED_SESSIONS,
                  ).expect("failed to create AGKE participant"),
                  keymanagement::AuthGraphParticipant::new(
                                  boring::crypto_trait_impls(),
                                  gw_ta.clone(),
                  ).expect("failed to create AGKM participant"),
                  Role::Sink);

            // Loop forever processing request messages.
            loop {
                let req_data: Vec<u8> = match in_rx.recv() {
                    Ok(data) => data,
                    Err(_) => {
                        error!("local TA failed to receive request!");
                        break;
                    }
                };
                // Route the received message to specific ta.
                let rsp_data = match req_data[0] {
                    AG_MESSAGE_PREFIX => ag_ta.process(&req_data[1..]),
                    GW_MESSAGE_PREFIX => gw_ta.borrow_mut().process(&req_data[1..]),
                    prefix => panic!("unexpected message prefix {prefix}!"),
                };
                // send back the response to caller of aidl interface
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
        Self { in_tx, out_rx }
    }

    fn execute_for(&mut self, prefix: u8, req_data: &[u8]) -> Vec<u8> {
        let mut prefixed_req = Vec::with_capacity(req_data.len() + 1);
        prefixed_req.push(prefix);
        prefixed_req.extend_from_slice(req_data);
        self.in_tx
            .send(prefixed_req)
            .expect("failed to send in request");
        self.out_rx.recv().expect("failed to receive response")
    }
}

/// Decalration and implementation of serialized channels for local ta.
pub struct AuthGraphChannel(pub Arc<Mutex<LocalTa>>);
impl SerializedChannel for AuthGraphChannel {
    const MAX_SIZE: usize = usize::MAX;
    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self
            .0
            .lock()
            .unwrap()
            .execute_for(AG_MESSAGE_PREFIX, req_data))
    }
}

pub struct GateWeaverChannel(pub Arc<Mutex<LocalTa>>);
impl SerializedChannel for GateWeaverChannel {
    const MAX_SIZE: usize = usize::MAX;
    fn execute(&self, req_data: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self
            .0
            .lock()
            .unwrap()
            .execute_for(GW_MESSAGE_PREFIX, req_data))
    }
}

/// An in-memory implementation of CredentialStore for a nonsecure ta.
pub struct InMemorySlot{
  slot_key: AesKey,
  attempts: u32,
  wait_time: u64,
}
pub struct InMemoryStore {
  store: [InMemorySlot; GW_MAX_SLOTS as usize],
}
impl InMemoryStore{
  pub fn new() -> Self{
    Self{store: array::from_fn(|_| InMemorySlot{
                                    slot_key: AesKey([0; AES_256_KEY_LEN]),
                                    attempts: 0, wait_time:0}),}
  }
}
impl CredentialStore for InMemoryStore {
    /// Return the number of slots supported by the credential store.
    fn get_capacity(&self) -> i32{
      GW_MAX_SLOTS
    }

    // reset the slot
    fn reset(&mut self, slot_id: i32) -> Result<(), wire::Error>{
      if !(slot_id < GW_MAX_SLOTS) {
        return Err(gw_err!(StorageError, "slot id is invalid"));
      }
      self.store[slot_id as usize].slot_key = AesKey([0; AES_256_KEY_LEN]);
      self.store[slot_id as usize].attempts = 0;
      self.store[slot_id as usize].wait_time = 0;
      Ok(())
    }
    // Create the credential key arc in the slot.
    fn create(
        &mut self,
        slot_id: i32,
        slot_key: AesKey
    ) -> Result<(), wire::Error>{
      if !(slot_id < GW_MAX_SLOTS) {
        return Err(gw_err!(StorageError, "slot id is invalid"));
      }
      self.store[slot_id as usize].slot_key = slot_key;
      self.store[slot_id as usize].attempts = 0;
      self.store[slot_id as usize].wait_time = 0;
      Ok(())
    }

    // read the slot data.
    fn get(&self, slot_id: i32) -> Result<(AesKey, u32, u64), wire::Error>{
      if !(slot_id < GW_MAX_SLOTS) {
        return Err(gw_err!(StorageError, "slot id is invalid"));
      }
      Ok((self.store[slot_id as usize].slot_key.clone(), self.store[slot_id as usize].attempts,self.store[slot_id as usize].wait_time))
    }
    // Set the attempts and wait time to new value in the slot.
    fn set(&mut self, slot_id: i32, attempts: u32, wait_time: u64) -> Result<(), wire::Error>{
      if !(slot_id < GW_MAX_SLOTS) {
        return Err(gw_err!(StorageError, "slot id is invalid"));
      }
      self.store[slot_id as usize].attempts = attempts;
      self.store[slot_id as usize].wait_time = wait_time;
      Ok(())
    }
}

/// This method generates common test id.
pub fn get_id() -> Result<(EcSignKey, Identity), wire::Error>{
  todo!()
}
