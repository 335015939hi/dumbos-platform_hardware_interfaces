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
    use secretkeeper_comm::data_types::error::SecretkeeperError;
    use secretkeeper_comm::data_types::request::Request;
    use secretkeeper_comm::data_types::request_response_impl::{
        GetVersionRequest, GetVersionResponse,
    };
    use secretkeeper_comm::data_types::response::Response;
    use secretkeeper_comm::data_types::packet::ResponsePacket;
    use android_hardware_security_secretkeeper::aidl::android::hardware::security::secretkeeper::ISecretkeeper::ISecretkeeper;

    const SECRETKEEPER_IDENTIFIER: &str =
        "android.hardware.security.secretkeeper.ISecretkeeper/nonsecure";
    const CURRENT_VERSION: u64 = 1;

    fn get_connection() -> binder::Strong<dyn ISecretkeeper> {
        binder::get_interface(SECRETKEEPER_IDENTIFIER).unwrap()
    }

    #[test]
    fn secret_management_get_version() {
        let secretkeeper = get_connection();
        let request = GetVersionRequest {};
        let request_packet = request.serialize_to_packet();
        let request_bytes = request_packet.serialize_to_bytes().unwrap();

        let response_bytes = secretkeeper
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::deserialize_from_bytes(&response_bytes).unwrap();
        assert!(!response_packet.is_error().unwrap());
        let get_version_response =
            *GetVersionResponse::deserialize_from_packet(response_packet).unwrap();
        assert_eq!(get_version_response.version, CURRENT_VERSION);
    }

    #[test]
    fn secret_management_malformed_request() {
        let secretkeeper = get_connection();
        let request = GetVersionRequest {};
        let request_packet = request.serialize_to_packet();
        let mut request_bytes = request_packet.serialize_to_bytes().unwrap();

        // Deform the request
        request_bytes[0] = !request_bytes[0];

        let response_bytes = secretkeeper
            .processSecretManagementRequest(&request_bytes)
            .unwrap();

        let response_packet = ResponsePacket::deserialize_from_bytes(&response_bytes).unwrap();
        assert!(response_packet.is_error().unwrap());
        let err = *SecretkeeperError::deserialize_from_packet(response_packet).unwrap();
        assert_eq!(err, SecretkeeperError::RequestMalformed);
    }
}
