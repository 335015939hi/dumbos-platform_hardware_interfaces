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

#![allow(unused_imports)]
#![allow(dead_code)]

mod ffi;

use android_hardware_bluetooth::aidl::android::hardware::bluetooth::{
    IBluetoothHci::IBluetoothHci, IBluetoothHciCallbacks::IBluetoothHciCallbacks, Status::Status
};
use binder::{
    BinderFeatures, ExceptionCode, Interface, Result as BinderResult, Strong,
};
use std::sync::{
    Arc, Mutex
};

enum State {
    Closed,
    Opened {
        ffi: ffi::Wrapper<HciHalCallbacks>,
        callbacks: Strong<dyn IBluetoothHciCallbacks>,
//        _death_recipient: DeathRecipient,
    },
}

pub struct HciHal {
    state: Arc<Mutex<State>>,
    callbacks: HciHalCallbacks,
}

struct HciHalCallbacks {
    state: Arc<Mutex<State>>,
}

impl Interface for HciHal {}

impl HciHal {
    pub fn new() -> Self {
        let state = Arc::new(Mutex::new(State::Closed));
        Self {
            callbacks: HciHalCallbacks::new(state.clone()),
            state: state,
        }
    }
}

impl IBluetoothHci for HciHal {

    fn close(&self) -> BinderResult<()> {
        let mut state = self.state.lock().unwrap();
        let State::Opened { ref ffi, .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        ffi.close();

        *state = State::Closed;
        Ok(())
    }

    fn initialize(&self, callbacks: &Strong<(dyn IBluetoothHciCallbacks)>) -> BinderResult<()> {
        let mut state = self.state.lock().unwrap();
        if matches!(*state, State::Opened{ .. }) {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        }

        let mut ffi = ffi::Wrapper::new();
        ffi.initialize(HciHalCallbacks::new(self.state.clone()));

        *state = State::Opened{ callbacks: callbacks.clone(), ffi: ffi };
        Ok(())
    }

    fn sendHciCommand(&self, data: &[u8]) -> BinderResult<()> {
        let state = self.state.lock().unwrap();
        let State::Opened { ref ffi, .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        ffi.send_command(data);

        Ok(())
    }

    fn sendAclData(&self, data: &[u8]) -> BinderResult<()> {
        let state = self.state.lock().unwrap();
        let State::Opened { ref ffi, .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        ffi.send_acl(data);

        Ok(())
    }

    fn sendScoData(&self, data: &[u8]) -> BinderResult<()> {
        let state = self.state.lock().unwrap();
        let State::Opened { ref ffi, .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        ffi.send_sco(data);

        Ok(())
    }

    fn sendIsoData(&self, data: &[u8]) -> BinderResult<()> {
        let state = self.state.lock().unwrap();
        let State::Opened { ref ffi, .. } = *state else {
            return Err(ExceptionCode::ILLEGAL_STATE.into());
        };

        ffi.send_iso(data);

        Ok(())
    }
}

impl HciHalCallbacks {
    fn new(state: Arc<Mutex<State>>) -> Self {
        Self {
            state: state,
        }
    }
}

impl ffi::Callbacks for HciHalCallbacks {

    fn initialization_complete(&self, status: ffi::Status) {
        let state = self.state.lock().unwrap();
        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.initializationComplete(status.into()).expect("Completing initialization");
        };
    }

    fn recv_event(&self, data: &[u8]) {
        let state = self.state.lock().unwrap();
        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.hciEventReceived(data).expect("Receiving Event Data");
        };
    }

    fn recv_acl(&self, data: &[u8]) {
        let state = self.state.lock().unwrap();
        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.aclDataReceived(data).expect("Receiving ACL Data");
        };
    }

    fn recv_sco(&self, data: &[u8]) {
        let state = self.state.lock().unwrap();
        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.scoDataReceived(data).expect("Receiving SCO Data");
        };
    }

    fn recv_iso(&self, data: &[u8]) {
        let state = self.state.lock().unwrap();
        if let State::Opened { ref callbacks, .. } = *state {
            callbacks.isoDataReceived(data).expect("Receiving ISO Data");
        };
    }

}

impl From<ffi::Status> for Status {
    fn from(value: ffi::Status) -> Self {
        match value {
            ffi::Status::Success => Status::SUCCESS,
            ffi::Status::AlreadyInitialized => Status::ALREADY_INITIALIZED,
            ffi::Status::UnableToOpenInterface => Status::UNABLE_TO_OPEN_INTERFACE,
            ffi::Status::HardwareInitializationError => Status::HARDWARE_INITIALIZATION_ERROR,
            ffi::Status::Unknown => Status::UNKNOWN
        }
    }
}
