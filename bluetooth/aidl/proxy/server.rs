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
    DeathRecipient, ExceptionCode, Interface, Result as BinderResult, Strong,
};
use std::sync::{ Arc, RwLock };

use crate::proxy::{ Proxy, Module };
use crate::ffi::{ Ffi, CInterface, CStatus, Callbacks, DataCallbacks };

enum State {
    Closed,
    Opened {
        proxy: Arc<Proxy<FfiCallbacks, ProxyCallbacks>>,
        _death_recipient: DeathRecipient,
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
    pub fn new(cintf: CInterface) -> Self {
        log::error!("new()...");
        Self {
            ffi: Arc::new(Ffi::new(cintf)),
            state: Arc::new(RwLock::new(State::Closed)),
        }
    }
}

impl IBluetoothHci for HciHalProxy {

    fn initialize(&self, callbacks: &Strong<dyn IBluetoothHciCallbacks>) -> BinderResult<()> {
        let mut state = self.state.write().unwrap();
        if matches!(*state, State::Opened { .. }) {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        }

        let proxy = Arc::new(Proxy::new(
            self.ffi.clone(),
            ProxyCallbacks::new(callbacks.clone())
        ));

        log::error!("initialize...");
        self.ffi.initialize(FfiCallbacks::new(callbacks.clone(), proxy.clone()));
        log::error!("initialize returned");

        *state = State::Opened {
            proxy: proxy.clone(),
            _death_recipient: {
                let (ffi, state) = (self.ffi.clone(), self.state.clone());
                DeathRecipient::new(move || {
                    log::info!("Bluetooth client has died");
                    *state.write().unwrap() = State::Closed;
                    ffi.close();
                })
            }
        };

        Ok(())
    }

    fn close(&self) -> BinderResult<()> {
        let mut state = self.state.write().unwrap();
        if matches!(*state, State::Closed) {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        *state = State::Closed;
        self.ffi.close();

        Ok(())
    }

    fn sendHciCommand(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_cmd(data);
        Ok(())
    }

    fn sendAclData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_acl(data);
        Ok(())
    }

    fn sendScoData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        proxy.out_sco(data);
        Ok(())
    }

    fn sendIsoData(&self, data: &[u8]) -> BinderResult<()> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
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

impl Callbacks for FfiCallbacks {
    fn initialization_complete(&self, status: CStatus) {
        self.callbacks.initializationComplete(status.into()).expect("Completing initialization");
    }
}

impl DataCallbacks for FfiCallbacks {
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

impl DataCallbacks for ProxyCallbacks {

    fn event_received(&self, data: &[u8]) {
        if let Err(e) = self.callbacks.hciEventReceived(data) {
            log::error!("Cannot send event to client: {:?}", e);
        }
    }

    fn acl_received(&self, data: &[u8]) {
        if let Err(e) = self.callbacks.aclDataReceived(data) {
            log::error!("Cannot send ACL to client: {:?}", e);
        }
    }

    fn sco_received(&self, data: &[u8]) {
        if let Err(e) = self.callbacks.scoDataReceived(data) {
            log::error!("Cannot send SCO to client: {:?}", e);
        }
    }

    fn iso_received(&self, data: &[u8]) {
        if let Err(e) = self.callbacks.isoDataReceived(data) {
            log::error!("Cannot send ISO to client: {:?}", e);
        }
    }
}

impl From<CStatus> for Status {
    fn from(value: CStatus) -> Self {
        match value {
            CStatus::Success => Status::SUCCESS,
            CStatus::AlreadyInitialized => Status::ALREADY_INITIALIZED,
            CStatus::UnableToOpenInterface => Status::UNABLE_TO_OPEN_INTERFACE,
            CStatus::HardwareInitializationError => Status::HARDWARE_INITIALIZATION_ERROR,
            CStatus::Unknown => Status::UNKNOWN
        }
    }
}
