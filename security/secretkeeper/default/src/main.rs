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
use log::{error, info, Level};
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::packet::{RequestPacket, ResponsePacket};
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse, Opcode, StoreSecretRequest, StoreSecretResponse,
    GetSecretRequest, GetSecretResponse,
};
use secretkeeper_comm::data_types::cbor_ser::CborBytesConversion;
use secretkeeper_comm::data_types::response::Response;
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::{
    BnSecretkeeper, BpSecretkeeper, ISecretkeeper,
};
use secretkeeper_core::SecretkeeperStore;
use std::sync::Arc;

const CURRENT_VERSION: u64 = 1;

// Developer note: I've got this chain by patching libdice_policy such that it dumps the
// dice_policy corresponding to a hypothetical dice_chain (which is hard-coded input in
// VTS secretkeeper_test_client.rs)
const HYPOTHETICAL_DICE_CHAIN: &str
 = "82a101008440a05834a3017374657374696e675f646963655f706f6c6963790274756e636f6e73747261696e65645f737472696e670346a1186419e9754464646566";

pub struct NonSecureSecretkeeper {
    // Authentication capable storage
    auth_store: Arc<SecretkeeperStore>,
}

impl Interface for NonSecureSecretkeeper {}

impl ISecretkeeper for NonSecureSecretkeeper {
    fn processSecretManagementRequest(&self, request: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self.process_opaque_request(request))
    }
}

impl NonSecureSecretkeeper {
    fn new() -> Self {
        let kv_store = secretkeeper_nonsecure::InMemoryStore::new();
        Self {
            auth_store: Arc::new(SecretkeeperStore::init(Arc::new(kv_store))),
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
        let response = GetVersionResponse::new(CURRENT_VERSION);
        Ok(response.serialize_to_packet())
    }

    fn process_store_secret_request(
        &self,
        request: RequestPacket,
    ) -> Result<ResponsePacket, SecretkeeperError> {
        let request = StoreSecretRequest::deserialize_from_packet(request)
            .map_err(|_| SecretkeeperError::RequestMalformed)?;
        self.auth_store.store(
            request.id().clone(),
            request.secret().clone(),
            request.sealing_policy().clone(),
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
        let secret = self.auth_store.get(
            request.id(),
            // TODO(b/291228560): Dice chain should be received during AuthgraphKeyExchange
            // instead of being hard-coded.
            &hex::decode(HYPOTHETICAL_DICE_CHAIN)
                .map_err(|_| SecretkeeperError::UnexpectedServerError)?,
            request.updated_sealing_policy().clone(),
        )?;
        let response = GetSecretResponse::new(secret);
        Ok(response.serialize_to_packet())
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
