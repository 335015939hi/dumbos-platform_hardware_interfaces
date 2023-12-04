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

use binder::{BinderFeatures, Interface};
use coset::CborSerializable;
use log::{error, info, Level};
use secretkeeper_comm::data_types::error::{Error, SecretkeeperError};
use secretkeeper_comm::data_types::packet::{RequestPacket, ResponsePacket};
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse, Opcode, StoreSecretRequest, StoreSecretResponse,
    GetSecretRequest, GetSecretResponse,
};
use secretkeeper_comm::data_types::Id;
use secretkeeper_comm::data_types::response::Response;
use secretkeeper_core::{AuthCapableStorage, KeyValueStore};
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::{
    BnSecretkeeper, BpSecretkeeper, ISecretkeeper,
};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

const CURRENT_VERSION: u64 = 1;

// Developer note: I've got this chain by patching libdice_policy such that it dumps the
// dice_policy corresponding to a hypothetical dice_chain (which is hard-coded input in
// VTS secretkeeper_test_client.rs)
const HYPOTHETICAL_DICE_CHAIN: &str =
    "82a101008440a05834a3017374657374696e675f646963655f706f6c6963790274756e636f6e737472616\
          96e65645f737472696e670346a1186419e9754464646566";

pub struct NonSecureSecretkeeper {
    // Authentication capable storage
    store: Arc<AuthCapableStorage>,
}

impl Interface for NonSecureSecretkeeper {}

impl ISecretkeeper for NonSecureSecretkeeper {
    fn processSecretManagementRequest(&self, request: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self.process_opaque_request(request))
    }

    fn deleteById(&self, id: &[u8]) -> binder::Result<()> {
        // TODO: maybe index using the internal bstr
        info!("Id, size {:?}", id.len());
        let id = Id::from_slice(id).unwrap();
        info!("Id.from_slice(id).unwrap().0.len(): {:?}", id.0.len());
        // TODO: dont ignore error
        let _ = self.store.delete_by_id(&id);
        Ok(())
    }

    fn deleteAll(&self) -> binder::Result<()> {
        // TODO: dont ignore error
        let _ = self.store.delete_all();
        Ok(())
    }
}

impl NonSecureSecretkeeper {
    fn new() -> Self {
        let kv_store = InMemoryStore::default();
        Self {
            store: Arc::new(AuthCapableStorage::init(Box::new(kv_store))),
        }
    }

    // A set of requests to Secretkeeper are 'opaque' - encrypted bytes with inner structure
    // described by CDDL. They need to be decrypted, deserialized and processed accordingly.
    fn process_opaque_request(&self, request: &[u8]) -> Vec<u8> {
        // TODO(b/291224769) The request will need to be decrypted & response need to be encrypted
        // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.
        self.process_opaque_request_unhandled_error(request)
            .unwrap_or_else(
                // SecretkeeperError is also a valid 'Response', serialize to a response packet.
                |sk_err| {
                    Response::serialize_to_packet(&sk_err)
                        .to_vec()
                        .expect("Panicking due to serialization failing")
                },
            )
    }

    fn process_opaque_request_unhandled_error(
        &self,
        request: &[u8],
    ) -> Result<Vec<u8>, SecretkeeperError> {
        let request_packet = RequestPacket::from_slice(request).map_err(|e| {
            error!("Failed to get Request packet from bytes: {:?}", e);
            SecretkeeperError::RequestMalformed
        })?;
        let response_packet = match request_packet
            .opcode()
            .map_err(|_| SecretkeeperError::RequestMalformed)?
        {
            Opcode::GetVersion => Self::process_get_version_request(request_packet)?,
            Opcode::StoreSecret => self.process_store_secret_request(request_packet)?,
            Opcode::GetSecret => self.process_get_secret_request(request_packet)?,
            _ => panic!("Unknown operation.."),
        };

        response_packet
            .to_vec()
            .map_err(|_| SecretkeeperError::UnexpectedServerError)
    }

    fn process_get_version_request(
        request: RequestPacket,
    ) -> Result<ResponsePacket, SecretkeeperError> {
        // Deserialization really just verifies the structural integrity of the request such
        // as args being empty.
        let _request = GetVersionRequest::deserialize_from_packet(request)
            .map_err(|_| SecretkeeperError::RequestMalformed)?;
        let response = GetVersionResponse {
            version: CURRENT_VERSION,
        };
        Ok(response.serialize_to_packet())
    }

    fn process_store_secret_request(
        &self,
        request: RequestPacket,
    ) -> Result<ResponsePacket, SecretkeeperError> {
        let request = StoreSecretRequest::deserialize_from_packet(request)
            .map_err(|_| SecretkeeperError::RequestMalformed)?;
        self.store.store(
            request.id,
            request.secret,
            request.sealing_policy,
            // TODO(b/291228560): Dice chain should be received during AuthgraphKeyExchange
            // instead of being hard-coded.
            &hex::decode(HYPOTHETICAL_DICE_CHAIN)
                .map_err(|_| SecretkeeperError::UnexpectedServerError)?,
        )?;
        let response = StoreSecretResponse {};
        Ok(response.serialize_to_packet())
    }

    fn process_get_secret_request(
        &self,
        request: RequestPacket,
    ) -> Result<ResponsePacket, SecretkeeperError> {
        let request = GetSecretRequest::deserialize_from_packet(request)
            .map_err(|_| SecretkeeperError::RequestMalformed)?;
        let secret = self.store.get(
            &request.id,
            // TODO(b/291228560): Dice chain should be received during AuthgraphKeyExchange
            // instead of being hard-coded.
            &hex::decode(HYPOTHETICAL_DICE_CHAIN)
                .map_err(|_| SecretkeeperError::UnexpectedServerError)?,
            request.updated_sealing_policy,
        )?;
        let response = GetSecretResponse { secret };
        Ok(response.serialize_to_packet())
    }
}

/// An in-memory implementation of KeyValueStore. Please note that this is entirely for
/// testing purposes. Refer to the documentation of `AuthCapableStorage` & Secretkeeper HAL for
/// persistence requirements.
#[derive(Default)]
pub struct InMemoryStore(Arc<Mutex<HashMap<Vec<u8>, Vec<u8>>>>);
impl KeyValueStore for InMemoryStore {
    fn store(&self, key: &[u8], val: &[u8]) -> Result<(), Error> {
        // This will overwrite the value if key is already present.
        error!("KeyValueStore storing {:?}", key);
        let _ = self.0.lock().unwrap().insert(key.to_vec(), val.to_vec());
        Ok(())
    }

    fn get(&self, key: &[u8]) -> Result<Option<Vec<u8>>, Error> {
        let db = self.0.lock().unwrap();
        error!("Getting key {:?}", key);

        let optional_val = db.get(key);
        Ok(optional_val.cloned())
    }

    fn delete_by_key(&self, key: &[u8]) -> Result<(), Error> {
        let _ = self.0.lock().unwrap().remove(key);
        Ok(())
    }
    fn delete_all(&self) -> Result<(), Error> {
        self.0.lock().unwrap().clear();
        Ok(())
    }
}

fn main() {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("NonSecureSecretkeeper")
            .with_min_level(Level::Info)
            .with_log_id(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    let service = NonSecureSecretkeeper::new();
    let service_binder = BnSecretkeeper::new_binder(service, BinderFeatures::default());
    let service_name = format!(
        "{}/nonsecure",
        <BpSecretkeeper as ISecretkeeper>::get_descriptor()
    );
    binder::add_service(&service_name, service_binder.as_binder()).unwrap_or_else(|e| {
        panic!(
            "Failed to register service {} because of {:?}.",
            service_name, e
        );
    });
    info!("Registered Binder service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
}
