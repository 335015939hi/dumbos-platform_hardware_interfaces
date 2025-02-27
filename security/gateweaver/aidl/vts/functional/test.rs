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

//! Tests of individual AuthGraph role (source or sink) functionality.

#![cfg(test)]

use authgraph_agkm_vts_test as vts_km;
use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyManagement::IAuthGraphKeyManagement,
    IAuthGraphKeyExchange::IAuthGraphKeyExchange,
    //Arc::Arc,
};
use android_hardware_security_gateweaver::aidl::android::hardware::security::gateweaver::{
    IGateWeaver::IGateWeaver, VerifyResponse::VerifyResponse, //GateWeaverConfig::GateWeaverConfig
    VerifySuccessResponse::VerifySuccessResponse,
    ReEnrollResponse::ReEnrollResponse, Error::Error,
};
use authgraph_core::{arc, key::AES_256_KEY_LEN};
use ciborium::value::{Value};
use coset::{AsCborValue, CborSerializable, CoseEncrypt0, Label};
use binder::StatusCode;

#[allow(dead_code)]
const AUTH_GRAPH_KM_GATEWEAVER: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyManagement/gateweaver";
const AUTH_GRAPH_KE_GATEWEAVER: &str =
    "android.hardware.security.authgraph.IAuthGraphKeyExchange/gateweaver";
const GATEWEAVER: &str =
    "android.hardware.security.gateweaver.IGateWeaver/nonsecure";// TODO change this to whatever is published gateweaver

/// Source id hash
pub const GW_SOURCE_ID_HASH_KEY: Label = Label::Int(-700011);
/// User Id
pub const GW_UID_KEY: Label = Label::Int(-700012);
/// Slot id
pub const GW_SLOT_ID_KEY: Label = Label::Int(-700013);

/// Retrieve the /gateweaver instance of AuthGraphKeyManagement, which supports both sink and source roles.
#[allow(dead_code)]
fn get_km_gw() -> Option<binder::Strong<dyn IAuthGraphKeyManagement>> {
    match binder::get_interface(AUTH_GRAPH_KM_GATEWEAVER) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyManagement/gateweaver: {e:?}"),
    }
}

/// Retrieve the /gateweaver instance of AuthGraphKeyExchange, which supports both sink and source roles.
fn get_ke_gw() -> Option<binder::Strong<dyn IAuthGraphKeyExchange>> {
    match binder::get_interface(AUTH_GRAPH_KE_GATEWEAVER) {
        Ok(ag) => Some(ag),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get AuthGraphKeyExchange/gateweaver: {e:?}"),
    }
}
/// Retrieve the /nonsecure instance of Gateweaver.
fn get_gw() -> Option<binder::Strong<dyn IGateWeaver>> {
    match binder::get_interface(GATEWEAVER) {
        Ok(gw) => Some(gw),
        Err(StatusCode::NAME_NOT_FOUND) => None,
        Err(e) => panic!("failed to get Gateweaver/nonsecure: {e:?}"),
    }
}
/// Macro to require availability of km service
#[allow(unused_macros)]
macro_rules! require_km {
    {} => {
        match get_km_gw() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /gateweaver impl found");
                return;
            }
        }
    }
}

/// Macro to require availability of ke service
#[allow(unused_macros)]
macro_rules! require_ke {
    {} => {
        match get_ke_gw() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no /gateweaver impl found");
                return;
            }
        }
    }
}

/// Macro to require availability of gw service
macro_rules! require_gw {
    {} => {
        match get_gw() {
            Some(v) => v,
            None => {
                eprintln!("Skipping test as no gateweaver instance found");
                return;
            }
        }
    }
}
#[test]
fn test_ag_gw_mainline() {
    todo!()
    // Then perform key management
//    let mut sink_ctx = vts_km::test_agkm_participant(false).expect("failed to create a local sink");

    //TODO replace the following with gateweaver ta
    // For default test device credential arc is always fixed.
//    let cred_arc = sink_ctx.cred_arc.clone();
//    vts_km::km_source::test_agkm_mainline(
//      &mut sink_ctx,
//      cred_arc,
//      src_out_arc,
//      sink_in_arc,
//      require_km!());
}

// In this test vts acts as sink and also orchestrator and tests the gateweaver.
#[allow(unreachable_code, unused_variables, unused_mut)]
#[test]
fn test_gw_api_success() {

    // TODO at this moment it is not known how can vts get source id being used by gateweaver.
    //  More checks pertaining to source id policy will be added once this is known.
    let protector_key = vec![0; AES_256_KEY_LEN];
    let slot_id = 0;
    let challenge = 1;
    // First perform key exchange
    let mut sink_ke = vts_km::test_agke_participant(false).expect("failed to create a local sink");
    let (src_out_arc, sink_in_arc) = vts_km::km_source::test_agke_mainline(&mut sink_ke, require_ke!());
    // create gw
    let gw_ta = require_gw!();
    // Test get config
    let config = gw_ta.getConfig().expect("config response should be valid");
    // Currently only gateweaver with internal secure clock are tests
    assert!(!config.timeRequired);
    // slots should be greater then 0.
    assert!(config.slots > 0);
    // Test create
    let create_resp =
        gw_ta.create(slot_id, &protector_key).expect("create response should be valid");
    let (uid, source_id_hash) = validate_create_resp(slot_id, &create_resp.arc);
    // Test verify
    let verify_resp = gw_ta
        .verify(&create_resp, &protector_key, challenge, None)
        .expect("verify response must be valid");
    match verify_resp {
        VerifyResponse::Success(ref resp) => {
            validate_success_resp(&uid, challenge, 0, resp)
        }
        VerifyResponse::Error(ref resp) => {
            panic!("Verify failed with error: {:?}", resp.errorCode)
        }
    }

    // Test re-enroll
    let enroll_resp = gw_ta
        .reEnroll(
            &create_resp,
            &protector_key,
            None,
            slot_id,
            &[1u8; AES_256_KEY_LEN],
        )
        .expect("re enroll response must be valid");
    let id_hash = validate_re_enroll_resp(slot_id, &uid, &enroll_resp);
    assert!(id_hash == source_id_hash, "source id hashes:{:?} \n {:?}",id_hash, source_id_hash);
    // Test invalidate
    gw_ta.invalidate(slot_id).expect("invalidate should not fail");
    let verify_resp = gw_ta
        .verify(&create_resp, &protector_key, challenge, None)
        .expect("verify response must be valid");
    match verify_resp {
        VerifyResponse::Success(ref resp) => panic!(
            "Expected invalid blob error - instead received status code: {:?}",
            resp.statusCode
        ),
        VerifyResponse::Error(ref resp) =>assert!(
            resp.errorCode == Error::INVALID_BLOB,
            "Un-expected error: {:?}", resp.errorCode),
    };
}

fn validate_re_enroll_resp(
    slot_id: i32,
    uid: &[u8],
    enroll_resp: &ReEnrollResponse,
) -> Vec<u8> {
    match enroll_resp {
        ReEnrollResponse::Success(ref resp) => {
            let prot_header = CoseEncrypt0::from_slice(&resp.newGWBlob.arc)
                .unwrap()
                .protected
                .header
                .rest;
            let head_values = extract_vals_from_header(&prot_header)
                .expect("enroll resp protected header values should be valid");
            assert!(
                head_values.slot_id.expect("slot id should be present in protected header")
                    == slot_id
            );
            assert!(head_values
                .source_id_hash.is_some(), "source id hash is absent in the protected header");

            assert!(
                head_values.uid.expect("uid shoud be present in protected header") == uid,
                "Uids don't match"
            );
            head_values.source_id_hash.unwrap()
        }
        ReEnrollResponse::Error(ref resp) => {
            panic!("Verified Failed with error: {:?}", resp.errorCode)
        }
    }
}

fn validate_success_resp(
    uid: &[u8],
    challenge: i64,
    status: i32,
    resp: &VerifySuccessResponse,
) {
    assert!(resp.statusCode == status, "wrong status code: {:?}", resp.statusCode);
    let prot_header =
        CoseEncrypt0::from_slice(&resp.credentialKey.arc).unwrap().protected.header.rest;
    let head_values = extract_vals_from_header(&prot_header)
        .expect("verify success resp protected header values should be valid");
    assert!(head_values.permissions.is_some(), "Permissions not defined in credential arc");
    assert!(head_values.limitations.is_some(), "Limitations not defined in credential arc");
    assert!(head_values.payload_type.is_some(), "Payload type not defined");
    assert!(
        head_values.payload_type.clone().unwrap() == arc::PayloadType::CredKey,
        "Invalid payload type: {:?}",
        head_values.payload_type
    );
    let perms = head_values.permissions.clone().unwrap();
    let limits = head_values.limitations.clone().unwrap();
    let uids = perms.uids.expect("UID should be present");
    assert!(uids.len() == 1, "Wrong number of uids in permissions");
    assert!(uids[0] == uid, "Wrong uid in permissions");
    let _source_id = perms.source_id.expect("Source Id should be present");
    let ch: [u8; 8] =
        limits.challenge.expect("Challenge must be present").try_into().unwrap_or_else(
            |v: Vec<u8>| panic!("Expected a Challenge of length 8 but it was {}", v.len()),
        );
    assert!(i64::from_be_bytes(ch) == challenge, "Challenge don't match");
}
#[allow(unreachable_code, unused_variables, unused_mut)]
fn validate_create_resp(slot_id: i32, resp: &[u8]) -> (Vec<u8>, Vec<u8>) {
    let prot_header =
            CoseEncrypt0::from_slice(resp).unwrap().protected.header.rest;
    let head_values = extract_vals_from_header(&prot_header)
            .expect("create resp protected header values should be valid");
    assert!(head_values.slot_id.expect("slot id should be present in protected header") == slot_id);
    assert!(head_values.uid.is_some(), "uid should be present in protected header");
    assert!(head_values.source_id_hash.is_some(), "source id hash is not defined in create response");
    (head_values.uid.unwrap(), head_values.source_id_hash.unwrap())
}

/// This is the structure to hold the protected header values read from
/// CoseEncrypt0.
#[derive(Default)]
pub struct ProtectedHeaderValues {
    /// Slot id which is gateweaver specific header value.
    pub slot_id: Option<i32>,
    /// Payload type used by any auth graph participant.
    pub payload_type: Option<arc::PayloadType>,
    /// SHA256 hash of source id which is gateweaver specific header value.
    pub source_id_hash: Option<Vec<u8>>,
    /// The uid which is GW specific header value.
    pub uid: Option<Vec<u8>>,
    /// timestamp of the arc generation
    pub timestamp: Option<u64>,
    /// permissions
    pub permissions: Option<arc::Permissions>,
    /// limitations
    pub limitations: Option<arc::Limitations>,
}
// Extract the capabilities from the arc
pub fn extract_vals_from_header(
    protected_headers: &[(Label, Value)],
) -> Result<ProtectedHeaderValues, String> {
    let mut header_vals = ProtectedHeaderValues::default();
    for (label, val) in protected_headers {
        match *label {
            arc::PERMISSIONS => {
                header_vals.permissions =
                    Some(arc::Permissions::from_cbor_value(val.clone())
                    .map_err(|e| format!("Could not decode permissions: {e:?}"))?);
            },
            arc::LIMITATIONS => {
                header_vals.limitations =
                    Some(arc::Limitations::from_cbor_value(val.clone())
                    .map_err(|e| format!("Could not decode limitations: {e:?}"))?);
            },
            arc::TIMESTAMP => {
                header_vals.timestamp = Some(
                    val.clone()
                        .as_integer()
                        .ok_or_else(||"Could not decode timestamp")?
                        .try_into()
                        .map_err(|e| format!("Could not decode timestamp int: {e:?}"))?);
            },
            arc::PAYLOAD_TYPE => {
                let payload_type: i32 = val
                    .clone()
                    .as_integer()
                    .ok_or_else(|| "Could not decode slot_id")?
                    .try_into()
                    .map_err(|e|format!("Could not decode slot_id int: {e:?}"))?;
                header_vals.payload_type = match payload_type {
                    1 => Some(arc::PayloadType::SecretKey),
                    2 => Some(arc::PayloadType::Arc),
                    3 => Some(arc::PayloadType::CredKey),
                    4 => Some(arc::PayloadType::Timestamp),
                    _ => return Err("Invalid Payload Type".to_string()),
                };
            },
            GW_SLOT_ID_KEY => {
                header_vals.slot_id = Some(
                    val.clone()
                        .as_integer()
                        .ok_or_else(|| "Could not decode slot_id")?
                        .try_into()
                        .map_err(|e| format!("Could not decode slot_id int: {e:?}"))?);
            },
            GW_SOURCE_ID_HASH_KEY => {
                header_vals.source_id_hash =
                    Some(val.as_bytes().cloned().ok_or_else(|| format!("Could not decode source_id_hash"))?);
            },
            GW_UID_KEY => {
                header_vals.uid =
                    Some(val.as_bytes().cloned().ok_or_else(||format!("Could not decode uid"))?);
            },
            _ => (), // do nothing
        }
    }
    Ok(header_vals)
}
