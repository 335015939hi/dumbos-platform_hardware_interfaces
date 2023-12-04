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

#[cfg(test)]
use binder::StatusCode;
use log::warn;
use secretkeeper_comm::data_types::error::SecretkeeperError;
use secretkeeper_comm::data_types::request::Request;
use secretkeeper_comm::data_types::request_response_impl::{
    GetVersionRequest, GetVersionResponse,
};
use secretkeeper_comm::data_types::{Id, ID_SIZE, Secret, SECRET_SIZE};
use secretkeeper_comm::data_types::response::Response;
use secretkeeper_comm::data_types::packet::{ResponsePacket, ResponseType};
use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;
use secretkeeper_comm::data_types::request_response_impl::StoreSecretRequest;
use secretkeeper_comm::data_types::request_response_impl::StoreSecretResponse;
use secretkeeper_comm::data_types::request_response_impl::GetSecretRequest;
use secretkeeper_comm::data_types::request_response_impl::GetSecretResponse;
use coset::CborSerializable;
const SECRETKEEPER_IDENTIFIER: &str =
    "android.hardware.security.secretkeeper.ISecretkeeper/nonsecure";
const CURRENT_VERSION: u64 = 1;

// TODO(b/291238565): This will change once serialization of dice_policy is changed & it switches
// to Explicit-key DiceCertChain
const HYPOTHETICAL_DICE_POLICY: &str = "a26776657273696f6e01756e6f64655f636f6e73747261696e74735f6c\
    6973748281830180a1010082830181017374657374696e675f646963655f706f6c69637983028203186419e975";
const ID_EXAMPLE: [u8; ID_SIZE] = [
    0xF1, 0xB2, 0xED, 0x3B, 0xD1, 0xBD, 0xF0, 0x7D, 0xE1, 0xF0, 0x01, 0xFC, 0x61, 0x71, 0xD3, 0x42,
    0xE5, 0x8A, 0xAF, 0x33, 0x6C, 0x11, 0xDC, 0xC8, 0x6F, 0xAE, 0x12, 0x5C, 0x26, 0x44, 0x6B, 0x86,
    0xCC, 0x24, 0xFD, 0xBF, 0x91, 0x4A, 0x54, 0x84, 0xF9, 0x01, 0x59, 0x25, 0x70, 0x89, 0x38, 0x8D,
    0x5E, 0xE6, 0x91, 0xDF, 0x68, 0x60, 0x69, 0x26, 0xBE, 0xFE, 0x79, 0x58, 0xF7, 0xEA, 0x81, 0x7D,
];
const ID_EXAMPLE_2: [u8; ID_SIZE] = [
    0x6A, 0xCC, 0xB1, 0xEB, 0xBB, 0xAB, 0xE3, 0xEA, 0x44, 0xBD, 0xDC, 0x75, 0x75, 0x7D, 0xC0, 0xE5,
    0xC7, 0x86, 0x41, 0x56, 0x39, 0x66, 0x96, 0x10, 0xCB, 0x43, 0x10, 0x79, 0x03, 0xDC, 0xE6, 0x9F,
    0x12, 0x2B, 0xEF, 0x28, 0x9C, 0x1E, 0x32, 0x46, 0x5F, 0xA3, 0xE7, 0x8D, 0x53, 0x63, 0xE8, 0x30,
    0x5A, 0x17, 0x6F, 0xEF, 0x42, 0xD6, 0x58, 0x7A, 0xF0, 0xCB, 0xD4, 0x40, 0x58, 0x96, 0x32, 0xF4,
];
const ID_NOT_STORED: [u8; ID_SIZE] = [
    0x56, 0xD0, 0x4E, 0xAA, 0xC1, 0x7B, 0x55, 0x6B, 0xA0, 0x2C, 0x65, 0x43, 0x39, 0x0A, 0x6C, 0xE9,
    0x1F, 0xD0, 0x0E, 0x20, 0x3E, 0xFB, 0xF5, 0xF9, 0x3F, 0x5B, 0x11, 0x1B, 0x18, 0x73, 0xF6, 0xBB,
    0xAB, 0x9F, 0xF2, 0xD6, 0xBD, 0xBA, 0x25, 0x68, 0x22, 0x30, 0xF2, 0x1F, 0x90, 0x05, 0xF3, 0x64,
    0xE7, 0xEF, 0xC6, 0xB6, 0xA0, 0x85, 0xC9, 0x40, 0x40, 0xF0, 0xB4, 0xB9, 0xD8, 0x28, 0xEE, 0x9C,
];
const SECRET_EXAMPLE: [u8; SECRET_SIZE] = [
    0xA9, 0x89, 0x97, 0xFE, 0xAE, 0x97, 0x55, 0x4B, 0x32, 0x35, 0xF0, 0xE8, 0x93, 0xDA, 0xEA, 0x24,
    0x06, 0xAC, 0x36, 0x8B, 0x3C, 0x95, 0x50, 0x16, 0x67, 0x71, 0x65, 0x26, 0xEB, 0xD0, 0xC3, 0x98,
];

fn get_connection() -> Option<binder::Strong<dyn ISecretkeeper>> {
    match binder::get_interface(SECRETKEEPER_IDENTIFIER) {
        Ok(sk) => Some(sk),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => {
            panic!(
                "unexpected error while fetching connection to Secretkeeper {:?}",
                e
            );
        }
    }
}

// TODO(b/2797757): Add tests that match different HAL defined objects (like request/response)
// with expected bytes.

#[test]
fn secret_management_get_version() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let request_bytes = request_packet.to_vec().unwrap();

    // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
    // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

    let response_bytes = secretkeeper
        .processSecretManagementRequest(&request_bytes)
        .unwrap();

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Success
    );
    let get_version_response =
        *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(get_version_response.version, CURRENT_VERSION);
}

#[test]
fn secret_management_malformed_request() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    let request = GetVersionRequest {};
    let request_packet = request.serialize_to_packet();
    let mut request_bytes = request_packet.to_vec().unwrap();

    // Deform the request
    request_bytes[0] = !request_bytes[0];

    // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
    // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

    let response_bytes = secretkeeper
        .processSecretManagementRequest(&request_bytes)
        .unwrap();

    let response_packet = ResponsePacket::from_slice(&response_bytes).unwrap();
    assert_eq!(
        response_packet.response_type().unwrap(),
        ResponseType::Error
    );
    let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
    assert_eq!(err, SecretkeeperError::RequestMalformed);
}

#[test]
fn secret_management_store_get_secret_found() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };

    let store_request = StoreSecretRequest {
        id: Id(ID_EXAMPLE),
        secret: Secret(SECRET_EXAMPLE),
        sealing_policy: hex::decode(HYPOTHETICAL_DICE_POLICY).unwrap(),
    };
    warn!("store_request {:?}", store_request);

    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let store_response = secretkeeper
        .processSecretManagementRequest(&store_request)
        .unwrap();
    let store_response = ResponsePacket::from_slice(&store_response).unwrap();

    assert_eq!(
        store_response.response_type().unwrap(),
        ResponseType::Success
    );
    // Really just checking that the response is indeed StoreSecretResponse
    let _ = StoreSecretResponse::deserialize_from_packet(store_response).unwrap();

    // Get the secret that was just stored
    let get_request = GetSecretRequest {
        id: Id(ID_EXAMPLE),
        updated_sealing_policy: None,
    };
    warn!("get_request {:?}", get_request);
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();

    let get_response = secretkeeper
        .processSecretManagementRequest(&get_request)
        .unwrap();
    let get_response = ResponsePacket::from_slice(&get_response).unwrap();
    assert_eq!(get_response.response_type().unwrap(), ResponseType::Success);
    let get_response = *GetSecretResponse::deserialize_from_packet(get_response).unwrap();
    assert_eq!(get_response.secret.0, SECRET_EXAMPLE);
}

#[test]
fn secret_management_store_get_secret_not_found() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };
    // Get the secret that was never stored
    let get_request = GetSecretRequest {
        id: Id(ID_NOT_STORED),
        updated_sealing_policy: None,
    };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();

    let get_response = secretkeeper
        .processSecretManagementRequest(&get_request)
        .unwrap();
    // Check for error!
    let get_response = ResponsePacket::from_slice(&get_response).unwrap();
    assert_eq!(get_response.response_type().unwrap(), ResponseType::Error);
    let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
    assert_eq!(err, SecretkeeperError::EntryNotFound);
}

#[test]
fn secretkeeper_store_delete_by_id() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };

    let store_request = StoreSecretRequest {
        id: Id(ID_EXAMPLE),
        secret: Secret(SECRET_EXAMPLE),
        sealing_policy: hex::decode(HYPOTHETICAL_DICE_POLICY).unwrap(),
    };

    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let store_response = secretkeeper
        .processSecretManagementRequest(&store_request)
        .unwrap();
    let store_response = ResponsePacket::from_slice(&store_response).unwrap();

    assert_eq!(
        store_response.response_type().unwrap(),
        ResponseType::Success
    );
    // Really just checking that the response is indeed StoreSecretResponse
    let _ = StoreSecretResponse::deserialize_from_packet(store_response).unwrap();

    secretkeeper
        .deleteById(&Id(ID_EXAMPLE).to_vec().unwrap())
        .unwrap();

    // Get the secret that was just stored & deleted
    let get_request = GetSecretRequest {
        id: Id(ID_EXAMPLE),
        updated_sealing_policy: None,
    };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();

    let get_response = secretkeeper
        .processSecretManagementRequest(&get_request)
        .unwrap();
    // Check for error!
    let get_response = ResponsePacket::from_slice(&get_response).unwrap();
    assert_eq!(get_response.response_type().unwrap(), ResponseType::Error);
    let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
    assert_eq!(err, SecretkeeperError::EntryNotFound);
}

#[test]
fn secretkeeper_store_delete_all() {
    let secretkeeper = match get_connection() {
        Some(sk) => sk,
        None => {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
    };

    let store_request = StoreSecretRequest {
        id: Id(ID_EXAMPLE),
        secret: Secret(SECRET_EXAMPLE),
        sealing_policy: hex::decode(HYPOTHETICAL_DICE_POLICY).unwrap(),
    };

    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let store_response = secretkeeper
        .processSecretManagementRequest(&store_request)
        .unwrap();
    let store_response = ResponsePacket::from_slice(&store_response).unwrap();

    assert_eq!(
        store_response.response_type().unwrap(),
        ResponseType::Success
    );
    // Really just checking that the response is indeed StoreSecretResponse
    let _ = StoreSecretResponse::deserialize_from_packet(store_response).unwrap();

    let store_request = StoreSecretRequest {
        id: Id(ID_EXAMPLE_2),
        secret: Secret(SECRET_EXAMPLE),
        sealing_policy: hex::decode(HYPOTHETICAL_DICE_POLICY).unwrap(),
    };

    let store_request = store_request.serialize_to_packet().to_vec().unwrap();
    let store_response = secretkeeper
        .processSecretManagementRequest(&store_request)
        .unwrap();
    let store_response = ResponsePacket::from_slice(&store_response).unwrap();

    assert_eq!(
        store_response.response_type().unwrap(),
        ResponseType::Success
    );
    // Really just checking that the response is indeed StoreSecretResponse
    let _ = StoreSecretResponse::deserialize_from_packet(store_response).unwrap();

    secretkeeper.deleteAll().unwrap();

    // Get the secret that was just stored before deleteAll
    let get_request = GetSecretRequest {
        id: Id(ID_EXAMPLE),
        updated_sealing_policy: None,
    };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();

    let get_response = secretkeeper
        .processSecretManagementRequest(&get_request)
        .unwrap();

    // Check for error!
    let get_response = ResponsePacket::from_slice(&get_response).unwrap();
    assert_eq!(get_response.response_type().unwrap(), ResponseType::Error);
    let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
    assert_eq!(err, SecretkeeperError::EntryNotFound);

    // Get the secret that was just stored before deleteAll
    let get_request = GetSecretRequest {
        id: Id(ID_EXAMPLE_2),
        updated_sealing_policy: None,
    };
    let get_request = get_request.serialize_to_packet().to_vec().unwrap();

    let get_response = secretkeeper
        .processSecretManagementRequest(&get_request)
        .unwrap();
    // Check for error!
    let get_response = ResponsePacket::from_slice(&get_response).unwrap();
    assert_eq!(get_response.response_type().unwrap(), ResponseType::Error);
    let err = *SecretkeeperError::deserialize_from_packet(get_response).unwrap();
    assert_eq!(err, SecretkeeperError::EntryNotFound);
}
