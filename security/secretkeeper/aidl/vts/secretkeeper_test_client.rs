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
mod tests {
    use binder::StatusCode;
    use log::warn;
    use secretkeeper_comm::data_types::error::SecretkeeperError;
    use secretkeeper_comm::data_types::request::Request;
    use secretkeeper_comm::data_types::request_response_impl::{
        GetVersionRequest, GetVersionResponse,
    };
    use secretkeeper_comm::data_types::response::Response;
    use secretkeeper_comm::data_types::packet::{ResponsePacket, ResponseType};
    use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;
    use secretkeeper_comm::data_types::request_response_impl::StoreSecretRequest;
    use secretkeeper_comm::data_types::request_response_impl::StoreSecretResponse;
    use secretkeeper_comm::data_types::request_response_impl::GetSecretRequest;
    use secretkeeper_comm::data_types::request_response_impl::GetSecretResponse;

    const SECRETKEEPER_IDENTIFIER: &str =
        "android.hardware.security.secretkeeper.ISecretkeeper/nonsecure";
    const CURRENT_VERSION: u64 = 1;
    // TODO: Dice policy is larger than the dice chain. This is unacceptable!
    // const HYPOTHETICAL_DICE_CHAIN: &str = "82a101008440a05834a3017374657374696e675f646963655f706f6c6963790274756e636f6e73747261696e65645f737472696e670346a1186419e9754464646566";
    const HYPOTHETICAL_DICE_POLICY: &str = "a26776657273696f6e01756e6f64655f636f6e73747261696e74735f6c6973748281830180a1010082830181017374657374696e675f646963655f706f6c69637983028203186419e975";
    // const HYPOTHETICAL_UPDATED_DICE_CHAIN:"82a101008440a0583da3017374657374696e675f646963655f706f6c69637902781c616e6f746865725f756e636f6e73747261696e65645f737472696e670346a1186419e9764464646566"
    const HYPOTHETICAL_UPDATED_DICE_POLICY: &str = "a26776657273696f6e01756e6f64655f636f6e73747261696e74735f6c6973748281830180a1010082830181017374657374696e675f646963655f706f6c69637983028203186419e976";
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

    #[test]
    fn secret_management_get_version() {
        let secretkeeper_opt = get_connection();
        if secretkeeper_opt.is_none() {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
        let secretkeeper = secretkeeper_opt.unwrap();
        let request = GetVersionRequest {};
        let request_packet = request.serialize_to_packet();
        let request_bytes = request_packet.into_bytes().unwrap();

        // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
        // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

        let response_bytes = secretkeeper
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::from_bytes(&response_bytes).unwrap();
        assert_eq!(
            response_packet.response_type().unwrap(),
            ResponseType::Success
        );
        let get_version_response =
            *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
        assert_eq!(get_version_response.version(), CURRENT_VERSION);
    }

    #[test]
    fn secret_management_malformed_request() {
        let secretkeeper_opt = get_connection();
        if secretkeeper_opt.is_none() {
            warn!("Secretkeeper HAL is unavailable, skipping test");
            return;
        }
        let secretkeeper = secretkeeper_opt.unwrap();
        let request = GetVersionRequest {};
        let request_packet = request.serialize_to_packet();
        let mut request_bytes = request_packet.into_bytes().unwrap();

        // Deform the request
        request_bytes[0] = !request_bytes[0];

        // TODO(b/291224769) The request will need to be encrypted & response need to be decrypted
        // with key & related artifacts pre-shared via Authgraph Key Exchange HAL.

        let response_bytes = secretkeeper
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::from_bytes(&response_bytes).unwrap();
        assert_eq!(
            response_packet.response_type().unwrap(),
            ResponseType::Error
        );
        let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
        assert_eq!(err, SecretkeeperError::RequestMalformed);
    }

    #[test]
    fn secret_management_store_get_secret() {
        let secretkeeper = get_connection();
        let request = StoreSecretRequest::new(
            (*b"sixty_four_bytes_in_a_sentences_can_make_it_really_really_longer").into(),
            (*b"thirty_two_bytes_long_sentences_").into(),
            b"meaningless_for_unit_test".to_vec(),
        );
        let request_packet = request.serialize_to_packet();
        let request_bytes = request_packet.into_bytes().unwrap();

        let response_bytes = secretkeeper
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::from_bytes(&response_bytes).unwrap();
        assert!(!response_packet.is_error().unwrap());
        // Really just sanity checking that the response is indeed StoreSecretResponse
        let _ = StoreSecretResponse::deserialize_from_packet(response_packet).unwrap();

        // Get the secret just stored
        let get_request = GetSecretRequest {
            id: b"something".to_vec(),
            updated_sealing_policy: None,
        };
        // Move some of the asserts to a common function
        let get_request_packet = get_request.serialize_to_packet();
        let get_request_bytes = get_request_packet.serialize_to_bytes().unwrap();

        let get_response_bytes = secretkeeper
            .processSecretManagementRequest(&get_request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::deserialize_from_bytes(&get_response_bytes).unwrap();
        assert!(!response_packet.is_error().unwrap());
        let get_secret_response =
            *GetSecretResponse::from_packet(response_packet).unwrap();
        assert_eq!(get_secret_response.secret(), b"thirty_two_bytes_long_sentences_"); // TODO should be a const
    }

    // TODO: At the moment, service has hardcoded dice chain.
    // Introduce auth test when multiple dice chain can be handled by server.
    // #[test]
    // fn secret_management_auth_fails() {
    //     let secretkeeper = get_connection();
    //     let request = StoreSecretRequest {
    //         id: b"something3".to_vec(),
    //         secret: b"now you know".to_vec(),
    //         sealing_policy: hex::decode(HYPOTHETICAL_UPDATED_DICE_POLICY).unwrap(),
    //     };
    //     let request_packet = request.serialize_to_packet();
    //     let request_bytes = request_packet.serialize_to_bytes().unwrap();

    //     let response_bytes = secretkeeper
    //         .processSecretManagementRequest(&request_bytes)
    //         .unwrap();

    //     let response_packet = ResponsePacket::deserialize_from_bytes(&response_bytes).unwrap();
    //     assert!(response_packet.is_error().unwrap());
    //     let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
    //     assert_eq!(err, SecretkeeperError::DicePolicyError);
    // }
}
