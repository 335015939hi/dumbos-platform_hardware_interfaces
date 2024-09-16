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

//! VTS tests for sinks
use super::*;
//use authgraph_core::{keymanagement, key};

/// Run AuthGraph tests against the provided sink, using a local test source implementation.
pub fn test(
    local_source_ctx: &mut crate::AgkmTestContext,
    authbound_arc_caps: Vec<u8>,
    src_out_ch: Vec<u8>,
    sink_in_ch: Vec<u8>,
    sink: binder::Strong<dyn IAuthGraphKeyManagement>
) {
    test_agkm_mainline(local_source_ctx,
                      authbound_arc_caps.clone(),
                      src_out_ch.clone(),
                      sink_in_ch.clone(),
                      sink.clone());
}

/// Perform mainline AuthGraph key management with the provided sink and local implementation.
pub fn test_agkm_mainline(
    local_source_ctx: &mut crate::AgkmTestContext,
    authbound_arc_caps: Vec<u8>,
    src_out_ch: Vec<u8>,
    sink_in_ch: Vec<u8>,
    sink: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
    let k_cred = &local_source_ctx.cred_arc;
    let source = &mut local_source_ctx.agkm;
    // Create k_s
    let k_s = source.create_arc(k_cred, None, true);
    assert!(k_s.is_ok(), "{:?}", k_s.err().unwrap());
    let k_s = k_s.unwrap();
    // Snap arc k_s
    let k_s_pbk = source.snap_arc(k_cred, &k_s);
    assert!(k_s_pbk.is_ok(), "{:?}", k_s_pbk.err().unwrap());
    let k_s_pbk = k_s_pbk.unwrap();
    // Create k_a
    let k_a = source.create_arc(&k_s_pbk, Some(&authbound_arc_caps), true);
    assert!(k_a.is_ok(), "{:?}", k_a.err().unwrap());
    let k_a = k_a.unwrap();
    // Snap arc k_a
    let k_a_pbk = source.snap_arc(&k_s_pbk, &k_a);
    assert!(k_a_pbk.is_ok(), "{:?}", k_a_pbk.err().unwrap());
    let k_a_pbk = k_a_pbk.unwrap();
    // Mint shared channel arc.
    let k_ch_k_a = source.mint_arc(&src_out_ch, &k_a_pbk);
    assert!(k_ch_k_a.is_ok(), "{:?}", k_ch_k_a.err().unwrap());
    let k_ch_k_a = k_ch_k_a.unwrap();
    // Snap the k_a arc on sink side
    let k_a_pbk = sink.snap(&Arc{arc:sink_in_ch}, &Arc{arc:k_ch_k_a});
    assert!(k_a_pbk.is_ok(), "{:?}", k_a_pbk.err().unwrap());
    let k_a_pbk = k_a_pbk.unwrap();
    // Create k_i
    let k_i = sink.create(&k_a_pbk, None, Role::SINK);
    assert!(k_i.is_ok(), "{:?}", k_i.err().unwrap());
    let k_i = k_i.unwrap();
    // Snap arc k_i
    let k_i_pbk = sink.snap(&k_a_pbk, &k_i);
    assert!(k_i_pbk.is_ok(), "{:?}", k_i_pbk.err().unwrap());
    let k_i_pbk = k_i_pbk.unwrap();
    // Create k_w
    let k_w = sink.create(&k_i_pbk, None, Role::SINK);
    assert!(k_w.is_ok(), "{:?}", k_w.err().unwrap());
    let k_w = k_w.unwrap();
    // Snap arc k_w
    let k_w_pbk = sink.snap(&k_i_pbk, &k_w);
    assert!(k_w_pbk.is_ok(), "{:?}", k_w_pbk.err().unwrap());
}

/// Perform mainline AuthGraph key exchange with the provided source.
/// Return the agreed AES keys in plaintext, together with the session ID.
pub fn test_agke_mainline(
    local_source: &mut ke::AuthGraphParticipant,
    sink: binder::Strong<dyn IAuthGraphKeyExchange>,
) -> (Vec<u8>, Vec<u8>) {
   // Step 1: create an ephemeral ECDH key at the (local) source.
    let source_init_info = local_source
        .create()
        .expect("failed to create() with local impl");

    // Step 2: pass the source's ECDH public key and other session info to the (remote) sink.
    let init_result = sink
        .init(
            &build_plain_pub_key(&source_init_info.ke_key.pub_key),
            &vec_to_identity(&source_init_info.identity),
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with remote impl");
    let sink_init_info = init_result.sessionInitiationInfo;
    let sink_pub_key = extract_plain_pub_key(&sink_init_info.key.pubKey);

    let sink_info = init_result.sessionInfo;
    assert!(!sink_info.sessionId.is_empty());

    // Step 3: pass the sink's ECDH public key and other session info to the (local) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = local_source
        .finish(
            &sink_pub_key.plainPubKey,
            &sink_init_info.identity.identity,
            &sink_info.signature.signature,
            &sink_init_info.nonce,
            sink_init_info.version,
            source_init_info.ke_key,
        )
        .expect("failed to finish() with local impl");
    assert!(!source_info.session_id.is_empty());
    let src_out_arc = source_info.shared_keys[1].clone();

    // Step 4: pass the (local) source's session ID signature back to the sink, so it can check it
    // and update the symmetric keys so they're marked as authentication complete.
    let sink_arcs = sink
        .authenticationComplete(
            &vec_to_signature(&source_info.session_id_signature),
            &sink_info.sharedKeys,
        )
        .expect("failed to authenticationComplete() with remote sink");
    let sink_in_arc = sink_arcs[0].arc.clone();
    (src_out_arc, sink_in_arc)
}