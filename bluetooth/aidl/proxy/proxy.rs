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

use crate::{ ffi, ffi::Ffi };
use std::sync::Arc;

pub trait Io {
    fn out_cmd(&self, data: &[u8]);
    fn out_acl(&self, data: &[u8]);
    fn out_iso(&self, data: &[u8]);
    fn out_sco(&self, data: &[u8]);

    fn in_evt(&self, data: &[u8]);
    fn in_acl(&self, data: &[u8]);
    fn in_sco(&self, data: &[u8]);
    fn in_iso(&self, data: &[u8]);
}

pub(crate) trait Callbacks {
    fn event_received(&self, data: &[u8]);
    fn acl_received(&self, data: &[u8]);
    fn sco_received(&self, data: &[u8]);
    fn iso_received(&self, data: &[u8]);
}

pub(crate) struct Proxy<T: ffi::Callbacks + Callbacks> {
    ffi: Arc<Ffi<T>>,
    cb: T,
}

impl<T: ffi::Callbacks + Callbacks> Proxy<T> {
    pub(crate) fn new(ffi: Arc<Ffi<T>>, cb: T) -> Self {
        Self { ffi: ffi, cb: cb }
    }
}

impl<T: ffi::Callbacks + Callbacks> Io for Proxy<T> {
    fn out_cmd(&self, data: &[u8]) { self.ffi.send_command(data); }
    fn out_acl(&self, data: &[u8]) { self.ffi.send_acl(data); }
    fn out_iso(&self, data: &[u8]) { self.ffi.send_iso(data); }
    fn out_sco(&self, data: &[u8]) { self.ffi.send_sco(data); }

    fn in_evt(&self, data: &[u8]) { Callbacks::event_received(&self.cb, data); }
    fn in_acl(&self, data: &[u8]) { Callbacks::acl_received(&self.cb, data); }
    fn in_sco(&self, data: &[u8]) { Callbacks::sco_received(&self.cb, data); }
    fn in_iso(&self, data: &[u8]) { Callbacks::iso_received(&self.cb, data); }
}
