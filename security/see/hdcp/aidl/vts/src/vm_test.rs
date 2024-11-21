/*
 * Copyright (C) 2024 The Android Open Source Project
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
mod vts_hal_hdcp_auth_control_target_test {
    use android_hardware_security_see_hdcp::aidl::android::hardware::security::see::hdcp::IHdcpAuthControl::IHdcpAuthControl;
    use android_hardware_security_see_hdcp::binder;
    use cfg_if::cfg_if;

    use binder::{StatusCode, Strong};
    // Android rust unit-test framework does not support the test macros yet
    // TODO: b/381871181 support rust unit-test framework for microdroid payload,
    // on-par with Trusty's
    cfg_if! {
        if #[cfg(not(target_os = "android"))] {
            // target_os = trusty (Trusty VM)
            use android_hardware_drm::aidl::android::hardware::drm::HdcpLevels::HdcpLevels;
            use android_hardware_drm::aidl::android::hardware::drm::HdcpLevel::HdcpLevel;
            use trusty_std::ffi::{CString, FallibleCString};
            use rpcbinder::RpcSession;
            use test::{assert_ok, expect};
        }
    }

    const HDCP_SERVICE_NAME: &str = "android.hardware.security.see.hdcp.IHdcpAuthControl/default";

    cfg_if! {
        if #[cfg(target_os = "android")] {
            // target_os = android (microdroid VM)
            fn connect() -> Result<Strong<dyn IHdcpAuthControl>, StatusCode> {
                binder::get_interface(HDCP_SERVICE_NAME)
            }
        } else {
            // target_os = trusty (Trusty VM)
            fn connect() -> Result<Strong<dyn IHdcpAuthControl>, StatusCode> {
                let port =
                    CString::try_new(HDCP_SERVICE_NAME).expect("Failed to allocate port name");
                RpcSession::new().setup_trusty_client(port.as_c_str())
            }
        }
    }

    #[test]
    fn get_hdcp_levels() {
        let hdcp_service = connect().expect("couldn't connect to HdcpAuthControl service");
        let levels = hdcp_service.getHdcpLevels();
        assert!(levels.is_ok());
        cfg_if! {
            if #[cfg(not(target_os = "android"))] {
            // target_os = trusty (Trusty VM)
            let HdcpLevels {
                    connectedLevel: connected_level,
                    maxLevel: max_level,
                } = levels.unwrap();
                expect!(connected_level >= HdcpLevel::HDCP_NONE);
                expect!(max_level <= HdcpLevel::HDCP_V2_3);
            }
        }
    }
    // TODO: remove the cfg_if blocks when microdroid payload supports
    // rust unittest macros (b/381871181)
    cfg_if! {
        if #[cfg(not(target_os = "android"))] {
            #[test]
            fn set_hdcp_levels() {
                let hdcp_service = connect().expect("couldn't connect to HdcpAuthControl service");
                let levels =
                    assert_ok!(hdcp_service.getHdcpLevels());
                let HdcpLevels {
                    connectedLevel: connected_level,
                    maxLevel: max_level,
                } = levels;
                expect!(connected_level >= HdcpLevel::HDCP_NONE);
                expect!(max_level <= HdcpLevel::HDCP_V2_3);
                // TODO: complete the test for setting HdcpLevels
            }
            test::init!();
        }
    }
}
