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

use core::{ ffi::c_void, slice };
use std::sync::{ Arc, RwLock };
use log;

#[repr(C)]
pub struct CCallbacks {
    handle: *const c_void,
    initialization_complete: extern "C" fn(*mut c_void, CStatus),
    event_received: extern "C" fn(*mut c_void, *const u8, usize),
    acl_received: extern "C" fn(*mut c_void, *const u8, usize),
    sco_received: extern "C" fn(*mut c_void, *const u8, usize),
    iso_received: extern "C" fn(*mut c_void, *const u8, usize),
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct CInterface {
    pub handle: *mut c_void,
    pub initialize: unsafe extern "C" fn(handle: *mut c_void, callbacks: *const CCallbacks),
    pub close: unsafe extern "C" fn(handle: *mut c_void),
    pub send_command: unsafe extern "C" fn (handle: *mut c_void, data: *const u8, len: usize),
    pub send_acl: unsafe extern "C" fn (handle: *mut c_void, data: *const u8, len: usize),
    pub send_sco: unsafe extern "C" fn (handle: *mut c_void, data: *const u8, len: usize),
    pub send_iso: unsafe extern "C" fn (handle: *mut c_void, data: *const u8, len: usize),
}

unsafe impl Send for CInterface {}
unsafe impl Sync for CInterface {}

#[repr(C)]
#[derive(Debug, PartialEq)]
#[allow(dead_code)]
pub(crate) enum CStatus {
    Success,
    AlreadyInitialized,
    UnableToOpenInterface,
    HardwareInitializationError,
    Unknown,
}

pub(crate) trait Callbacks : DataCallbacks {
    fn initialization_complete(&self, status: CStatus);
}

pub(crate) trait DataCallbacks {
    fn event_received(&self, data: &[u8]);
    fn acl_received(&self, data: &[u8]);
    fn sco_received(&self, data: &[u8]);
    fn iso_received(&self, data: &[u8]);
}

pub(crate) struct Ffi<T: Callbacks> {
    intf: CInterface,
    wrapper: Arc<RwLock<Option<T>>>,
}

impl<T: Callbacks> Ffi<T> {

    pub(crate) fn new(intf: CInterface) -> Self {
        Self { intf: intf, wrapper: Arc::new(RwLock::new(None)) }
    }

    pub(crate) fn initialize(&self, client: T) {
        unsafe {
            self.set_client(client);
            let wrapper = Arc::into_raw(self.wrapper.clone());
            (self.intf.initialize)(self.intf.handle, &CCallbacks::new(wrapper));
            let _ = Arc::from_raw(wrapper);
        }
    }

    pub(crate) fn send_command(&self, data: &[u8]) {
        unsafe {
            (self.intf.send_command)(self.intf.handle, data.as_ptr(), data.len());
        }
    }

    pub(crate) fn send_acl(&self, data: &[u8]) {
        unsafe {
            (self.intf.send_acl)(self.intf.handle, data.as_ptr(), data.len());
        }
    }

    pub(crate) fn send_iso(&self, data: &[u8]) {
        unsafe {
            (self.intf.send_iso)(self.intf.handle, data.as_ptr(), data.len());
        }
    }

    pub(crate) fn send_sco(&self, data: &[u8]) {
        unsafe {
            (self.intf.send_sco)(self.intf.handle, data.as_ptr(), data.len());
        }
    }

    pub(crate) fn close(&self) {
        unsafe {
            (self.intf.close)(self.intf.handle);
            self.remove_client();
        }
    }

    fn set_client(&self, client: T) {
        *self.wrapper.write().unwrap() = Some(client);
    }

    fn remove_client(&self) {
        *self.wrapper.write().unwrap() = None;
    }
}

impl CCallbacks {

    fn new<T: Callbacks>(handle: *const RwLock<Option<T>>) -> Self {
        Self {
            handle: handle.cast(),
            initialization_complete: Self::initialization_complete::<T>,
            event_received: Self::event_received::<T>,
            acl_received: Self::acl_received::<T>,
            sco_received: Self::sco_received::<T>,
            iso_received: Self::iso_received::<T>,
        }
    }

    fn unwrap_client<T: Callbacks, F: FnOnce(&T)>(handle: *mut c_void, f: F) {
        unsafe {
            let wrapper: Arc<RwLock<Option<T>>> = Arc::from_raw(handle.cast());
            if let Some(client) = &*wrapper.read().unwrap() {
                f(client);
            } else {
                log::error!("FFI Callback called in bad state");
            }
            let _ = Arc::into_raw(wrapper);
        }
    }

    extern "C" fn initialization_complete<T: Callbacks>(handle: *mut c_void, status: CStatus) {
        Self::unwrap_client(handle,
            |client: &T| { client.initialization_complete(status) }
        );
    }

    extern "C" fn event_received<T: Callbacks>(handle: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            Self::unwrap_client(handle,
                |client: &T| client.event_received(slice::from_raw_parts(data, len))
            );
        }
    }

    extern "C" fn acl_received<T: Callbacks>(handle: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            Self::unwrap_client(handle,
                |client: &T| client.acl_received(slice::from_raw_parts(data, len))
            );
        }
    }

    extern "C" fn sco_received<T: Callbacks>(handle: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            Self::unwrap_client(handle,
                |client: &T| client.sco_received(slice::from_raw_parts(data, len))
            );
        }
    }

    extern "C" fn iso_received<T: Callbacks>(handle: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            Self::unwrap_client(handle,
                |client: &T| client.iso_received(slice::from_raw_parts(data, len))
            );
        }
    }
}
