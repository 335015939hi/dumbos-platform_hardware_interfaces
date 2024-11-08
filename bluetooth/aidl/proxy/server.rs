// Copyright 2024, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use android_hardware_bluetooth::aidl::android::hardware::bluetooth::{
    IBluetoothHci::IBluetoothHci, IBluetoothHciCallbacks::IBluetoothHciCallbacks, Status::Status
};
use binder::{
    ExceptionCode, Interface, Result as BinderResult, Strong,
};
use std::sync::{ Arc, RwLock };

use crate::{ proxy, proxy::Proxy, proxy::Module, ffi, ffi::Ffi };

enum State {
    Closed,
    Opened {
        proxy: Arc<Proxy<FfiCallbacks, ProxyCallbacks>>,
//        _death_recipient: DeathRecipient,
    },
}

pub struct HciHalProxy {
    ffi: Arc<Ffi<FfiCallbacks>>,
    state: Arc<RwLock<State>>,
}

struct FfiCallbacks {
    callbacks: Strong<dyn IBluetoothHciCallbacks>,
    proxy: Arc<Proxy<FfiCallbacks, ProxyCallbacks>>,
}

struct ProxyCallbacks {
    callbacks: Strong<dyn IBluetoothHciCallbacks>,
}

impl Interface for HciHalProxy {}

impl HciHalProxy {
    pub fn new(cintf: ffi::CInterface) -> Self {
        Self {
            ffi: Arc::new(Ffi::new(cintf)),
            state: Arc::new(RwLock::new(State::Closed)),
        }
    }
}

impl IBluetoothHci for HciHalProxy {

    fn initialize(&self, callbacks: &Strong<dyn IBluetoothHciCallbacks>) -> BinderResult<()> {
        let mut state = self.state.write().unwrap();
        let State::Opened { .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        let proxy = Arc::new(Proxy::new(
            self.ffi.clone(),
            ProxyCallbacks::new(callbacks.clone())
        ));

        self.ffi.initialize(FfiCallbacks::new(callbacks.clone(), proxy.clone()));
        *state = State::Opened{ proxy: proxy };

        Ok(())
    }

    fn close(&self) -> BinderResult<()> {
        let mut state = self.state.write().unwrap();
        let State::Closed = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        *state = State::Closed;
        self.ffi.close();

        Ok(())
    }

    fn sendHciCommand(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_cmd(data);
        Ok(())
    }

    fn sendAclData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_acl(data);
        Ok(())
    }

    fn sendScoData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_sco(data);
        Ok(())
    }

    fn sendIsoData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_iso(data);
        Ok(())
    }
}

impl FfiCallbacks {
    fn new(callbacks: Strong<dyn IBluetoothHciCallbacks>,
           proxy: Arc<Proxy<FfiCallbacks, ProxyCallbacks>>) -> Self {
        Self { callbacks: callbacks, proxy: proxy }
    }
}

impl ffi::Callbacks for FfiCallbacks {

    fn initialization_complete(&self, status: ffi::CStatus) {
        self.callbacks.initializationComplete(status.into()).expect("Completing initialization");
    }

    fn event_received(&self, data: &[u8]) {
        self.proxy.in_evt(data);
    }

    fn acl_received(&self, data: &[u8]) {
        self.proxy.in_acl(data);
    }

    fn sco_received(&self, data: &[u8]) {
        self.proxy.in_sco(data);
    }

    fn iso_received(&self, data: &[u8]) {
        self.proxy.in_iso(data);
    }
}

impl ProxyCallbacks {
    fn new(callbacks: Strong<dyn IBluetoothHciCallbacks>) -> Self {
        Self { callbacks: callbacks }
    }
}

impl proxy::Callbacks for ProxyCallbacks {

    fn event_received(&self, data: &[u8]) {
        self.callbacks.hciEventReceived(data).expect("Receiving Event");
    }

    fn acl_received(&self, data: &[u8]) {
        self.callbacks.aclDataReceived(data).expect("Receiving ACL");
    }

    fn sco_received(&self, data: &[u8]) {
        self.callbacks.scoDataReceived(data).expect("Receiving SCO");
    }

    fn iso_received(&self, data: &[u8]) {
        self.callbacks.isoDataReceived(data).expect("Receiving ISO");
    }
}

impl From<ffi::CStatus> for Status {
    fn from(value: ffi::CStatus) -> Self {
        match value {
            ffi::CStatus::Success => Status::SUCCESS,
            ffi::CStatus::AlreadyInitialized => Status::ALREADY_INITIALIZED,
            ffi::CStatus::UnableToOpenInterface => Status::UNABLE_TO_OPEN_INTERFACE,
            ffi::CStatus::HardwareInitializationError => Status::HARDWARE_INITIALIZATION_ERROR,
            ffi::CStatus::Unknown => Status::UNKNOWN
        }
    }
}
