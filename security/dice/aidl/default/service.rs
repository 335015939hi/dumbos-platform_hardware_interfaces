// Copyright 2021, The Android Open Source Project
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

//! Main entry point for the android.hardware.security.dice service.

use diced::{
    dice,
    hal_node::{DiceArtifacts, DiceDevice, ResidentHal, UpdatableDiceArtifacts},
};
use anyhow::Result;
use serde::{Deserialize, Serialize};
use std::panic;
use std::sync::Arc;

static DICE_HAL_SERVICE_NAME: &str = "android.hardware.security.dice.IDiceDevice/default";

#[derive(Debug, Serialize, Deserialize, Clone)]
struct InsecureSerializableArtifacts {
    cdi_attest: [u8; dice::CDI_SIZE],
    cdi_seal: [u8; dice::CDI_SIZE],
    cert_chain: Vec<Vec<u8>>,
}

impl DiceArtifacts for InsecureSerializableArtifacts {
    fn cdi_attest(&self) -> &[u8; dice::CDI_SIZE] {
        &self.cdi_attest
    }
    fn cdi_seal(&self) -> &[u8; dice::CDI_SIZE] {
        &self.cdi_seal
    }
    fn cert_chain(&self) -> Vec<Vec<u8>> {
        self.cert_chain.clone()
    }
}

impl UpdatableDiceArtifacts for InsecureSerializableArtifacts {
    fn with_artifacts<F, T>(&self, f: F) -> Result<T>
    where
        F: FnOnce(&dyn DiceArtifacts) -> Result<T>,
    {
        f(self)
    }
    fn update(self, new_artifacts: &impl DiceArtifacts) -> Result<Self> {
        Ok(Self {
            cdi_attest: *new_artifacts.cdi_attest(),
            cdi_seal: *new_artifacts.cdi_seal(),
            cert_chain: new_artifacts.cert_chain(),
        })
    }
}

fn main() {
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("android.hardware.security.dice")
            .with_min_level(log::Level::Debug),
    );
    // Redirect panic messages to logcat.
    panic::set_hook(Box::new(|panic_info| {
        log::error!("{}", panic_info);
    }));

    // Saying hi.
    log::info!("android.hardware.security.dice is starting.");

    let hal_impl = Arc::new(
        ResidentHal::new(InsecureSerializableArtifacts {
            cdi_attest: [0u8; dice::CDI_SIZE],
            cdi_seal: [1u8; dice::CDI_SIZE],
            cert_chain: vec![vec![]],
        })
        .expect("Failed to create ResidentHal implementation."),
    );

    let hal = DiceDevice::new_as_binder(hal_impl).expect("Failed to construct hal service.");

    binder::add_service(DICE_HAL_SERVICE_NAME, hal.as_binder())
        .expect("Failed to register IDiceDevice Service");

    log::info!("Joining thread pool now.");
    binder::ProcessState::join_thread_pool();
}
