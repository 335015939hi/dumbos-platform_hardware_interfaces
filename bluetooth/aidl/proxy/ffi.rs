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

use core:: { ffi::c_void, slice };
use std::sync::Arc;

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
#[derive(Debug)]
#[allow(dead_code)]
pub(crate) enum CStatus {
    Success,
    AlreadyInitialized,
    UnableToOpenInterface,
    HardwareInitializationError,
    Unknown,
}

pub(crate) trait Callbacks {
    fn initialization_complete(&self, status: CStatus);
    fn event_received(&self, data: &[u8]);
    fn acl_received(&self, data: &[u8]);
    fn sco_received(&self, data: &[u8]);
    fn iso_received(&self, data: &[u8]);
}

pub(crate) struct Ffi<T: Callbacks> {
    intf: CInterface,
    client: Arc<T>,
}

impl<T: Callbacks> Ffi<T> {

    pub(crate) fn new(intf: CInterface, client: T) -> Self {
        Self { intf: intf, client: Arc::new(client) }
    }

    pub(crate) fn initialize(&self) {
        unsafe {
            let client = Arc::into_raw(self.client.clone());
            (self.intf.initialize)(self.intf.handle, &CCallbacks::new(client));
            let _ = Arc::from_raw(client);
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
        }
    }
}

impl CCallbacks {

    fn new<T: Callbacks>(client: *const T) -> Self {
        Self {
            handle: client.cast(),
            initialization_complete: Self::initialization_complete::<T>,
            event_received: Self::event_received::<T>,
            acl_received: Self::acl_received::<T>,
            sco_received: Self::sco_received::<T>,
            iso_received: Self::iso_received::<T>,
        }
    }

    extern "C" fn initialization_complete<T: Callbacks>(client: *mut c_void, status: CStatus) {
        unsafe {
            let client: Arc<T> = Arc::from_raw(client.cast());
            client.initialization_complete(status);
            let _ = Arc::into_raw(client);
        }
    }

    extern "C" fn event_received<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Arc<T> = Arc::from_raw(client.cast());
            dbg!(Arc::<T>::strong_count(&client));
            client.event_received(slice::from_raw_parts(data, len));
            let _ = Arc::into_raw(client);
        }
    }

    extern "C" fn acl_received<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Arc<T> = Arc::from_raw(client.cast());
            client.acl_received(slice::from_raw_parts(data, len));
            let _ = Arc::into_raw(client);
        }
    }

    extern "C" fn sco_received<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Arc<T> = Arc::from_raw(client.cast());
            client.sco_received(slice::from_raw_parts(data, len));
            let _ = Arc::into_raw(client);
        }
    }

    extern "C" fn iso_received<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Arc<T> = Arc::from_raw(client.cast());
            client.iso_received(slice::from_raw_parts(data, len));
            let _ = Arc::into_raw(client);
        }
    }
}
