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

use crate::{ proxy, proxy::Proxy, proxy::Io, ffi, ffi::Ffi };

use android_hardware_bluetooth::aidl::android::hardware::bluetooth::{
    IBluetoothHci::IBluetoothHci, IBluetoothHciCallbacks::IBluetoothHciCallbacks, Status::Status
};
use binder::{
    ExceptionCode, Interface, Result as BinderResult, Strong,
};
use log;
use std::sync::{ Arc, RwLock };

enum State {
    Closed,
    Opened {
        ffi: Arc<Ffi<HciHalCallbacks>>,
        proxy: Arc<Proxy<HciHalCallbacks>>,
//        _death_recipient: DeathRecipient,
    },
}

pub struct HciHalProxy {
    state: Arc<RwLock<State>>,
    cintf: ffi::CInterface,
}

#[derive(Clone)]
struct HciHalCallbacks {
    state: Arc<RwLock<State>>,
    callbacks: Strong<dyn IBluetoothHciCallbacks>,
}

impl Interface for HciHalProxy {}

impl HciHalProxy {
    pub fn new(cintf: ffi::CInterface) -> Self {
        Self {
            state: Arc::new(RwLock::new(State::Closed)),
            cintf: cintf,
        }
    }

    fn proxy(&self) -> BinderResult<Arc<Proxy<HciHalCallbacks>>> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };
        Ok(proxy.clone())
    }
}

impl IBluetoothHci for HciHalProxy {

    fn initialize(&self, callbacks: &Strong<(dyn IBluetoothHciCallbacks)>) -> BinderResult<()> {
        let ffi = {
            let mut state = self.state.write().unwrap();
            if matches!(*state, State::Opened{ .. }) {
                return Err(ExceptionCode::ILLEGAL_STATE.into());
            }

            let callbacks = HciHalCallbacks::new(self.state.clone(), callbacks.clone());
            let ffi = Arc::new(Ffi::new(self.cintf, callbacks.clone()));
            let proxy = Arc::new(Proxy::new(ffi.clone(), callbacks));

            *state = State::Opened{ ffi: ffi.clone(), proxy: proxy };
            ffi
        };

        ffi.initialize();

        Ok(())
    }

    fn close(&self) -> BinderResult<()> {
        let ffi = {
            let State::Opened { ref ffi, .. } = *self.state.read().unwrap() else {
                return Err(ExceptionCode::ILLEGAL_STATE.into());
            };
            ffi.clone()
        };

        ffi.close();
        *self.state.write().unwrap() = State::Closed;

        Ok(())
    }

    fn sendHciCommand(&self, data: &[u8]) -> BinderResult<()> {
        self.proxy()?.out_cmd(data);
        Ok(())
    }

    fn sendAclData(&self, data: &[u8]) -> BinderResult<()> {
        self.proxy()?.out_acl(data);
        Ok(())
    }

    fn sendScoData(&self, data: &[u8]) -> BinderResult<()> {
        self.proxy()?.out_sco(data);
        Ok(())
    }

    fn sendIsoData(&self, data: &[u8]) -> BinderResult<()> {
        self.proxy()?.out_iso(data);
        Ok(())
    }
}

impl HciHalCallbacks {
    fn new(state: Arc<RwLock<State>>, callbacks: Strong<dyn IBluetoothHciCallbacks>) -> Self {
        Self {
            state: state,
            callbacks: callbacks
        }
    }

    fn proxy(&self) -> Option<Arc<Proxy<HciHalCallbacks>>> {
        let State::Opened { ref proxy, .. } = *self.state.read().unwrap() else {
            return None;
        };
        Some(proxy.clone())
    }
}

impl ffi::Callbacks for HciHalCallbacks {

    fn initialization_complete(&self, status: ffi::CStatus) {
        self.callbacks.initializationComplete(status.into()).expect("Completing initialization");
    }

    fn event_received(&self, data: &[u8]) {
        let Some(proxy) = self.proxy() else {
            log::error!("Trashing event received in bad state");
            return;
        };

        proxy.in_evt(data);
    }

    fn acl_received(&self, data: &[u8]) {
        let Some(proxy) = self.proxy() else {
            log::error!("Trashing ACL data received in bad state");
            return;
        };

        proxy.in_acl(data);
    }

    fn sco_received(&self, data: &[u8]) {
        let Some(proxy) = self.proxy() else {
            log::error!("Trashing SCO data received in bad state");
            return;
        };

        proxy.in_sco(data);
    }

    fn iso_received(&self, data: &[u8]) {
        let Some(proxy) = self.proxy() else {
            log::error!("Trashing ISO data received in bad state");
            return;
        };

        proxy.in_iso(data);
    }
}

impl proxy::Callbacks for HciHalCallbacks {

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
