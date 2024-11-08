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

#[repr(C)]
#[derive(Debug)]
pub enum Status {
    Success,
    AlreadyInitialized,
    UnableToOpenInterface,
    HardwareInitializationError,
    Unknown,
}

pub trait Callbacks {
    fn initialization_complete(&self, status: Status);
    fn recv_event(&self, data: &[u8]);
    fn recv_acl(&self, data: &[u8]);
    fn recv_sco(&self, data: &[u8]);
    fn recv_iso(&self, data: &[u8]);
}

pub struct Wrapper<T> {
    handle: *mut c_void,
    client: Option<Box<T>>,
}

extern "C" {
    fn hal_initialize(callbacks: *const CCallbacks, handle: *mut *mut c_void);
    fn hal_close(handle: *mut c_void);
    fn hal_send_command(handle: *mut c_void, data: *const u8, len: usize);
    fn hal_send_acl(handle: *mut c_void, data: *const u8, len: usize);
    fn hal_send_sco(handle: *mut c_void, data: *const u8, len: usize);
    fn hal_send_iso(handle: *mut c_void, data: *const u8, len: usize);
}

impl<T: Callbacks> Wrapper<T> {

    pub fn new() -> Self {
        Wrapper { handle: std::ptr::null_mut(), client: None }
    }

    pub fn initialize(&mut self, client: T) {
        unsafe {
            let client = Box::into_raw(Box::new(client));
            hal_initialize(&CCallbacks::new::<T>(client.cast()), &mut self.handle);
            self.client = Some(Box::from_raw(client));
        }
    }

    pub fn close(&self) {
        unsafe {
            hal_close(self.handle);
        }
    }

    pub fn send_command(&self, data: &[u8]) {
        unsafe {
            hal_send_command(self.handle, data.as_ptr(), data.len());
        }
    }

    pub fn send_acl(&self, data: &[u8]) {
        unsafe {
            hal_send_acl(self.handle, data.as_ptr(), data.len());
        }
    }

    pub fn send_iso(&self, data: &[u8]) {
        unsafe {
            hal_send_iso(self.handle, data.as_ptr(), data.len());
        }
    }

    pub fn send_sco(&self, data: &[u8]) {
        unsafe {
            hal_send_sco(self.handle, data.as_ptr(), data.len());
        }
    }
}

unsafe impl<T> Send for Wrapper<T> {}
unsafe impl<T> Sync for Wrapper<T> {}

#[repr(C)]
struct CCallbacks {
    handle: *const c_void,
    initialization_complete: extern "C" fn(*mut c_void, Status),
    recv_event: extern "C" fn(*mut c_void, *const u8, usize),
    recv_acl: extern "C" fn(*mut c_void, *const u8, usize),
    recv_sco: extern "C" fn(*mut c_void, *const u8, usize),
    recv_iso: extern "C" fn(*mut c_void, *const u8, usize),
}

impl CCallbacks {
    fn new<T: Callbacks>(client: *const c_void) -> Self {
        Self {
            handle: client,
            initialization_complete: Self::initialization_complete::<T>,
            recv_event: Self::recv_event::<T>,
            recv_acl: Self::recv_acl::<T>,
            recv_sco: Self::recv_sco::<T>,
            recv_iso: Self::recv_iso::<T>,
        }
    }

    extern "C" fn initialization_complete<T: Callbacks>(client: *mut c_void, status: Status) {
        unsafe {
            let client: Box<T> = Box::from_raw(client.cast());
            client.initialization_complete(status);
            let _ = Box::into_raw(client);
        }
    }

    extern "C" fn recv_event<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Box<T> = Box::from_raw(client.cast());
            client.recv_event(slice::from_raw_parts(data, len));
            let _ = Box::into_raw(client);
        }
    }

    extern "C" fn recv_acl<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Box<T> = Box::from_raw(client.cast());
            client.recv_acl(slice::from_raw_parts(data, len));
            let _ = Box::into_raw(client);
        }
    }

    extern "C" fn recv_sco<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Box<T> = Box::from_raw(client.cast());
            client.recv_sco(slice::from_raw_parts(data, len));
            let _ = Box::into_raw(client);
        }
    }

    extern "C" fn recv_iso<T: Callbacks>(client: *mut c_void, data: *const u8, len: usize) {
        unsafe {
            let client: Box<T> = Box::from_raw(client.cast());
            client.recv_iso(slice::from_raw_parts(data, len));
            let _ = Box::into_raw(client);
        }
    }
}
