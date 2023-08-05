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

use android_hardware_secretkeeper::binder::{BinderFeatures, Interface, Result as BinderResult};
use android_logger::Config;
use binder;
use log::{info, Level};
use std::sync::Mutex;

use android_hardware_secretkeeper::aidl::android::hardware::secretkeeper::ISecretkeeper::{
    BnSecretkeeper, BpSecretkeeper, ISecretkeeper,
};

#[derive(Debug, Default)]
pub struct NonSecureSecretkeeper {
    store: Mutex<GlobalStore>,
}

impl Interface for NonSecureSecretkeeper {}
impl ISecretkeeper for NonSecureSecretkeeper {
    fn store(&self, payload: &[u8]) -> BinderResult<()> {
        let mut global_store = self.store.lock().unwrap();
        // TODO(b/291224769):  Payload is a cbor value which contains the secret.
        // Extract it out of the payload instead of storing the whole payload.
        let _ = global_store.set(payload)?;
        Ok(())
    }

    fn read(&self, _payload: &[u8]) -> BinderResult<Vec<u8>> {
        // Payload contains information required for authorizing the client.
        // NonSecureSecretkeeper does not have to worry about it.
        let global_store = self.store.lock().unwrap();
        global_store.get()
    }
}
fn main() {
    android_logger::init_once(
        Config::default()
            .with_tag("NonSecureSecretkeeper")
            .with_min_level(Level::Info)
            .with_log_id(android_logger::LogId::System),
    );
    let service = NonSecureSecretkeeper::default();
    let service_binder = BnSecretkeeper::new_binder(service, BinderFeatures::default());
    binder::add_service(
        &format!(
            "{}/nonsecure",
            <BpSecretkeeper as ISecretkeeper>::get_descriptor()
        ),
        service_binder.as_binder(),
    )
    .expect("Failed to register service?");
    info!("Registered Binder service, joining threadpool.");
    binder::ProcessState::join_thread_pool();
}

#[derive(Debug, Default)]
struct GlobalStore {
    // TODO(b/291224769): Support per-client secret instead of global one.
    held_secret: Vec<u8>,
}

impl GlobalStore {
    fn set(&mut self, payload: &[u8]) -> BinderResult<()> {
        self.held_secret = payload.to_vec();
        Ok(())
    }
    fn get(&self) -> BinderResult<Vec<u8>> {
        Ok(self.held_secret.clone())
    }
}
