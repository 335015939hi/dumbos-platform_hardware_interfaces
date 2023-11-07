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

use android_logger::Config;
use binder::{BinderFeatures, Interface};
use log::{error, info, Level};
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::packet::{RequestPacket, ResponsePacket};
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse, Opcode,
};
use secretkeeper_comm::data_types::response::Response;
use std::sync::{Arc, Mutex};

use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::{
    BnSecretkeeper, BpSecretkeeper, ISecretkeeper,
};
use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyExchange::IAuthGraphKeyExchange,
};

const CURRENT_VERSION: u64 = 1;

#[derive(Debug)]
pub struct NonSecureSecretkeeper {
    authgraph: binder::Strong<dyn IAuthGraphKeyExchange>,
}

impl Interface for NonSecureSecretkeeper {}

impl ISecretkeeper for NonSecureSecretkeeper {
    fn processSecretManagementRequest(&self, request: &[u8]) -> binder::Result<Vec<u8>> {
        Ok(self.process_secret_management_request_inner(request))
    }
    fn getAuthGraphKE(&self) -> binder::Result<binder::Strong<dyn IAuthGraphKeyExchange>> {
        Ok(self.authgraph.clone())
    }
}

impl NonSecureSecretkeeper {
    fn new() -> Self {
        let local_authgraph_ta = Arc::new(Mutex::new(authgraph_nonsecure::LocalTa::new()));
        let authgraph = authgraph_hal::service::AuthGraphService::new_as_binder(local_authgraph_ta);
        Self { authgraph }
    }
    fn process_secret_management_request_inner(&self, request: &[u8]) -> Vec<u8> {
        // TODO(b/291224769) The request will need to be decrypted & response need to be encrypted
        // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.
        self.process_secret_management_request_unhandled_error(request)
            .unwrap_or_else(
                // SecretkeeperError is also a valid 'Response', serialize to a response packet.
                |sk_err| {
                    Response::serialize_to_packet(&sk_err)
                        .to_bytes()
                        .expect("Panicking due to serialization failing")
                },
            )
    }

    fn process_secret_management_request_unhandled_error(
        &self,
        request: &[u8],
    ) -> Result<Vec<u8>, SecretkeeperError> {
        let request_packet = RequestPacket::from_bytes(request).map_err(|e| {
            error!("Failed to get Request packet from bytes: {}", e);
            SecretkeeperError::RequestMalformed
        })?;
        let response_packet = match request_packet
            .opcode()
            .map_err(|_| SecretkeeperError::RequestMalformed)?
        {
            Opcode::GetVersion => Self::process_get_version_request(request_packet)?,
            _ => todo!("TODO(b/291224769): Unimplemented operations"),
        };

        response_packet
            .to_bytes()
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
}

fn main() {
    android_logger::init_once(
        Config::default()
            .with_tag("NonSecureSecretkeeper")
            .with_min_level(Level::Info)
            .with_log_id(android_logger::LogId::System),
    );
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
