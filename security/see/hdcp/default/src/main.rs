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

//! Trusty implementation of the HdcpAuthControl HAL.

use log::{error, info, LevelFilter};
use hdcp_hal::HdcpAuthControlService;
use std::sync::{Arc, Mutex};
use android_hardware_security_see_hdcp::aidl::android::hardware::security::see::hdcp::IHdcpAuthControl::{
    BnHdcpAuthControl, IHdcpAuthControl,
};

fn main() {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("HdcpAuthControlService")
            .with_max_level(LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));


    let service = HdcpAuthControlService::new_as_binder();
    let service_name = format!(
        "{}/default",
        <BpHdcpAuthControl as IHdcpAuthControl>::get_descriptor()
    );
    // binder::add_service(&service_name, service.as_binder()).unwrap_or_else(|e| {
    //     panic!("Failed to register service {service_name} because of {e:?}.",);
    // });
    // info!("Registered Binder service, joining threadpool.");
    // binder::ProcessState::join_thread_pool();
}
