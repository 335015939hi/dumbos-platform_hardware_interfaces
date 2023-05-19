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

// `dm` module implements part of the `device-mapper` ioctl interfaces. It currently supports
// creation and deletion of the mapper device. It doesn't support other operations like querying
// the status of the mapper device. And there's no plan to extend the support unless it is
// required.
//
// Why in-house development? [`devicemapper`](https://crates.io/crates/devicemapper) is a public
// Rust implementation of the device mapper APIs. However, it doesn't provide any abstraction for
// the target-specific tables. User has to manually craft the table. Ironically, the library
// provides a lot of APIs for the features that are not required for `apkdmverity` such as listing
// the device mapper block devices that are currently listed in the kernel. Size is an important
// criteria for Microdroid.

#![allow(missing_docs)]
#![cfg_attr(test, allow(unused))]

use log::{info, Level};
use android_logger::Config;

use binder;

use android_hardware_rpmb::aidl::android::hardware::rpmb::IRpmb::IRpmb;
use android_hardware_rpmb::aidl::android::hardware::rpmb::RpmbMessageFrame::RpmbMessageFrame;
use android_hardware_rpmb::aidl::android::hardware::rpmb::IRpmb::BnRpmb;
use android_hardware_rpmb::binder::{
    BinderFeatures, Interface, Result as BinderResult,
};

const LOG_TAG: &str = "MockRpmbService";

#[derive(Debug, Default)]
pub struct MockRpmbService {
    write_counter: i32,
}

impl Interface for MockRpmbService {}

impl IRpmb for MockRpmbService {
    fn program_key(&self, _request_frames: &[RpmbMessageFrame], _response_frames: &mut Vec<RpmbMessageFrame>)
            -> BinderResult<()>{
        info!("Write counter is is {}", self.write_counter);
        // TODO: Do something
        Ok(())
    }

}

fn main() {
    android_logger::init_once(
        Config::default()
            .with_tag(LOG_TAG)
            .with_min_level(Level::Info)
            .with_log_id(android_logger::LogId::System),
    );

    let service = MockRpmbService {write_counter: 0};
    let service_binder = BnRpmb::new_binder(
        service,
        BinderFeatures::default(),
    );

    binder::add_service(
        &format!("{}/default", IRpmb::BpRpmb::get_descriptor()),
        service_binder.as_binder()).expect("Failed to register service?");
    info!("Registered Binder service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
}
