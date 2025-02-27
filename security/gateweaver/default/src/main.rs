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

//! Non-secure implementation of the GateWeaver HAL.

use log::{error, info, LevelFilter};
use gateweaver::GateWeaverService;
use gateweaver_nonsecure::{AuthGraphChannel, GateWeaverChannel, LocalTa};
use std::sync::{Arc, Mutex};
use android_hardware_security_gateweaver::aidl::android::hardware::security::gateweaver::IGateWeaver::{
    BpGateWeaver, IGateWeaver,
};

use android_hardware_security_authgraph::aidl::android::hardware::security::authgraph::{
    IAuthGraphKeyExchange::{IAuthGraphKeyExchange,BpAuthGraphKeyExchange},
    IAuthGraphKeyManagement::{IAuthGraphKeyManagement,BpAuthGraphKeyManagement},
};
use authgraph_km_hal::service::AuthGraphService;

fn main() {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("gateweaver-hal-nonsecure")
            .with_max_level(LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));
    info!("Insecure Gateweaver HAL service is starting.");

    // Create channels
    let ta = Arc::new(Mutex::new(LocalTa::new()));
    let ke_channel = AuthGraphChannel(ta.clone());
    let km_channel = AuthGraphChannel(ta.clone());
    let gw_channel = GateWeaverChannel(ta.clone());

    // Create services
    let ke_service = AuthGraphService::new_as_binder(ke_channel);
    let km_service = AuthGraphService::new_km_as_binder(km_channel);
    let gw_service = GateWeaverService::new_as_binder(gw_channel);

    // create service names
    let ke_service_name = format!(
      "{}/gateweaver",
      <BpAuthGraphKeyExchange as IAuthGraphKeyExchange>::get_descriptor()
    );
    let km_service_name = format!(
          "{}/gateweaver",
          <BpAuthGraphKeyManagement as IAuthGraphKeyManagement>::get_descriptor()
        );
    let gw_service_name = format!(
      "{}/nonsecure",
      <BpGateWeaver as IGateWeaver>::get_descriptor()
    );

    // Add services
    binder::add_service(&ke_service_name, ke_service.as_binder()).unwrap_or_else(|e| {
        panic!("Failed to register service {ke_service_name} because of {e:?}.",);
    });
    binder::add_service(&km_service_name, km_service.as_binder()).unwrap_or_else(|e| {
        panic!("Failed to register service {km_service_name} because of {e:?}.",);
    });
    binder::add_service(&gw_service_name, gw_service.as_binder()).unwrap_or_else(|e| {
        panic!("Failed to register service {gw_service_name} because of {e:?}.",);
    });

    info!("Registered Nonsecure GateWeaver service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
    info!("Nonsecure Gateweaver service is terminating."); // should not reach here
    ()
}
