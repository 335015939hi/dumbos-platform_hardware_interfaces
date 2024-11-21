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

//! mock implementation of a HdcpAuthControl service
//! similar to all see services, this mock implementation is expected to
//! run in a TEE (e.g. Trusty TEE); and be invoked from an nvf pVM

use android_hardware_security_see_hdcp::aidl::android::hardware::security::see::hdcp::IHdcpAuthControl::IHdcpAuthControl;
use android_hardware_security_see_hdcp::binder;
use android_hardware_drm::aidl::android::hardware::drm::HdcpLevels::HdcpLevels;
use android_hardware_drm::aidl::android::hardware::drm::HdcpLevel::HdcpLevel;
use cfg_if::cfg_if;

use binder::{StatusCode, Strong};
cfg_if! {
    if #[cfg(not(target_os = "android"))] {
        // target_os = trusty (Trusty VM)
        use trusty_std::ffi::{CString, FallibleCString};
        use rpcbinder::RpcSession;
    }
}

/// The `IHdcpAuthControl` implementation.
pub struct HdcpAuthControl;

impl binder::Interface for HdcpAuthControl {}

impl IHdcpAuthControl for HdcpAuthControl {
    fn wishHappyBirthday(&self, name: &str, years: i32) -> binder::Result<String> {
        Ok(format!("Happy Birthday {name}, congratulations with the {years} years!"))
    }
}
