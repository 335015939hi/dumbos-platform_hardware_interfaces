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

//! VTS tests for sources
use super::*;


/// Run AuthGraph key management tests against the provided source, using a local test sink implementation.
pub fn test(
    local_sink_ctx: &mut crate::AgkmTestContext,
    k_cred: Vec<u8>,
    src_out_ch: Vec<u8>,
    sink_in_ch: Vec<u8>,
    source: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
    test_agkm_mainline(local_sink_ctx,
                       k_cred.clone(),
                       src_out_ch.clone(),
                       sink_in_ch.clone(),
                       source.clone());
}

/// Perform mainline AuthGraph key management with the provided source and local implementation.
pub fn test_agkm_mainline(
    local_sink_ctx: &mut crate::AgkmTestContext,
    k_cred: Vec<u8>,
    src_out_ch: Vec<u8>,
    sink_in_ch: Vec<u8>,
    source: binder::Strong<dyn IAuthGraphKeyManagement>,
) {
    let sink = &mut local_sink_ctx.agkm;
    // Create k_s
    let k_s = source.create(&Arc{arc:k_cred.clone()}, None, Role::SOURCE);
    assert!(k_s.is_ok(), "{:?}", k_s.err().unwrap());
    let k_s = k_s.unwrap();
    // Snap arc k_s
    let k_s_pbk = source.snap(&Arc{arc:k_cred.clone()}, &k_s);
    assert!(k_s_pbk.is_ok(), "{:?}", k_s_pbk.err().unwrap());
    let k_s_pbk = k_s_pbk.unwrap();
    // Create k_a
    let k_a = source.create(&k_s_pbk,
              Some(&Capability{capabilities: local_sink_ctx.authbound_arc_caps.clone()}),
              Role::SOURCE);
    assert!(k_a.is_ok(), "{:?}", k_a.err().unwrap());
    let k_a = k_a.unwrap();
    // Snap arc k_a
    let k_a_pbk = source.snap(&k_s_pbk, &k_a);
    assert!(k_a_pbk.is_ok(), "{:?}", k_a_pbk.err().unwrap());
    let k_a_pbk = k_a_pbk.unwrap();
    // Mint shared channel arc.
    let k_ch_k_a = source.mint(&Arc{arc:src_out_ch}, &k_a_pbk);
    assert!(k_ch_k_a.is_ok(), "{:?}", k_ch_k_a.err().unwrap());
    let k_ch_k_a = k_ch_k_a.unwrap();
    // Snap the k_a arc on sink side
    let k_a_pbk = sink.snap_arc(&sink_in_ch, &k_ch_k_a.arc);
    assert!(k_a_pbk.is_ok(), "{:?}", k_a_pbk.err().unwrap());
    let k_a_pbk = k_a_pbk.unwrap();
    // Create k_i
    let k_i = sink.create_arc(&k_a_pbk, None, false);
    assert!(k_i.is_ok(), "{:?}", k_i.err().unwrap());
    let k_i = k_i.unwrap();
    // Snap arc k_i
    let k_i_pbk = sink.snap_arc(&k_a_pbk, &k_i);
    assert!(k_i_pbk.is_ok(), "{:?}", k_i_pbk.err().unwrap());
    let k_i_pbk = k_i_pbk.unwrap();
    // Create k_w
    let k_w = sink.create_arc(&k_i_pbk, None, false);
    assert!(k_w.is_ok(), "{:?}", k_w.err().unwrap());
    let k_w = k_w.unwrap();
    // Snap arc k_w
    let k_w_pbk = sink.snap_arc(&k_i_pbk, &k_w);
    assert!(k_w_pbk.is_ok(), "{:?}", k_w_pbk.err().unwrap());
}

/// Perform mainline AuthGraph key exchange with the provided source.
/// Return the agreed AES keys in plaintext, together with the session ID.
pub fn test_agke_mainline(
    local_sink: &mut ke::AuthGraphParticipant,
    source: binder::Strong<dyn IAuthGraphKeyExchange>,
) -> (Vec<u8>, Vec<u8>) {
   // Step 1: create an ephemeral ECDH key at the (remote) source.
    let source_init_info = source
        .create()
        .expect("failed to create() with remote impl");
    assert!(source_init_info.key.pubKey.is_some());
    assert!(source_init_info.key.arcFromPBK.is_some());
    let source_pub_key = extract_plain_pub_key(&source_init_info.key.pubKey);

    // Step 2: pass the source's ECDH public key and other session info to the (local) sink.
    let init_result = local_sink
        .init(
            &source_pub_key.plainPubKey,
            &source_init_info.identity.identity,
            &source_init_info.nonce,
            source_init_info.version,
        )
        .expect("failed to init() with local impl");
    let sink_init_info = init_result.session_init_info;
    let sink_pub_key = sink_init_info
        .ke_key
        .pub_key
        .expect("expect pub_key to be populated");

    let sink_info = init_result.session_info;
    assert!(!sink_info.session_id.is_empty());

    // Step 3: pass the sink's ECDH public key and other session info to the (remote) source, so it
    // can calculate the same pair of symmetric keys.
    let source_info = source
        .finish(
            &PubKey::PlainKey(PlainPubKey {
                plainPubKey: sink_pub_key,
            }),
            &Identity {
                identity: sink_init_info.identity,
            },
            &vec_to_signature(&sink_info.session_id_signature),
            &sink_init_info.nonce,
            sink_init_info.version,
            &source_init_info.key,
        )
        .expect("failed to finish() with remote impl");
    assert!(!source_info.sessionId.is_empty());
    let src_out_arc = source_info.sharedKeys[1].arc.clone();

    // Step 4: pass the (remote) source's session ID signature back to the sink, so it can check it
    // and update the symmetric keys so they're marked as authentication complete.
    let sink_arcs = local_sink
        .authentication_complete(&source_info.signature.signature, sink_info.shared_keys)
        .expect("failed to authenticationComplete() with local sink");
    let sink_in_arc = sink_arcs[0].clone();
    (src_out_arc, sink_in_arc)
}

