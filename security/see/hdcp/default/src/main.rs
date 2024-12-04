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

//! Trusty mock implementation of the HdcpAuthControl HAL.
//! similar to all see services, this mock implementation is expected to
//! run in a TEE (e.g. Trusty TEE); and be invoked from an AVF pVM

use log::{error, info, LevelFilter};
use std::sync::{Arc, Mutex};
use android_hardware_security_see_hdcp::aidl::android::hardware::security::see::hdcp::IHdcpAuthControl::{
    IHdcpAuthControl, HalErrorCode, PendingHdcpLevelResult
};
use android_hardware_security_see_hdcp::aidl::android::hardware::security::see::hdcp::MockIHdcpAuthControl;
use android_hardware_drm::aidl::android::hardware::drm::HdcpLevels::HdcpLevels;
use android_hardware_drm::aidl::android::hardware::drm::HdcpLevel::HdcpLevel;

// fn r#getHdcpLevels<'a, >(&'a self) -> binder::Result<crate::mangled::_7_android_8_hardware_3_drm_10_HdcpLevels> { self.0.r#getHdcpLevels() }
// fn r#trySetHdcpLevel<'a, >(&'a self, _arg_level: crate::mangled::_7_android_8_hardware_3_drm_9_HdcpLevel) -> binder::Result<()> { self.0.r#trySetHdcpLevel(_arg_level) }
// fn r#getPendingHdcpLevel<'a, >(&'a self) -> binder::Result<crate::mangled::_7_android_8_hardware_8_security_3_see_4_hdcp_16_IHdcpAuthControl_22_PendingHdcpLevelResult> { self.0.r#getPendingHdcpLevel() }
const HDCP_SERVICE_NAME: &str = "android.hardware.security.see.hdcp.IHdcpAuthControl/default";


/// Macro used to create a `HwCryptoError::HalError` by providing the AIDL `HalErrorCode` and a
/// message: `hwcrypto_err!(UNSUPPORTED, "unsupported operation")`
macro_rules! hdcp_err {
    { $error_code:ident, $($arg:tt)+ } => {
        $crate::HdcpError {
                code: $crate::HalErrorCode::$error_code,
                file: std::file!(),
                line: std::line!(),
                message: alloc::format!("{}",std::format_args!($($arg)+)),
        }
    };
}

/// Base Error type for HwCrypto library.
#[derive(Debug)]
pub enum HdcpError {
    /// HwCrypto library native error
    HalError { code: i32, file: &'static str, line: u32, message: String },
}

impl From<HdcpError> for binder::Status {
    fn from(e: HdcpError) -> Self {
        match e {
            HdcpError::HalError { code, file, line, message } => {
                let msg = CString::new(
                    format!("HWCrypto error on {}:{}: {}", file, line, message).as_str(),
                )
                .unwrap();
                binder::Status::new_service_specific_error(code, Some(&msg))
            }
        }
    }
}

fn main() {
    let mut mock = MockIHdcpAuthControl::new();
    // mock.expect_getHdcpLevels().returning(|| Ok(()));
    // mock.expect_trySetHdcpLevel().returning(|| Ok(()));
    // mock.expect_getPendingHdcpLevel().returning(|| Ok(()));

    let cfg = PortCfg::new(HDCP_SERVICE_NAME.to_str().expect("should not happen, valid utf-8"))
        .map_err(|e| {
            hdcp_err!(
                GENERIC_ERROR,
                "could not create port config for {:?}: {:?}",
                HDCP_SERVICE_NAME,
                e
            )
        })?
        .allow_ta_connect()
        .allow_ns_connect();

    let manager = Manager::<_, _, 1, 4>::new_unbuffered(mock, cfg)
        .map_err(|e| hdcp_err!(GENERIC_ERROR, "could not create Bn manager: {:?}", e))?;

    manager
        .run_event_loop()
        .map_err(|e| hdcp_err!(GENERIC_ERROR, "Bn manager received error: {:?}", e))
}
