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

//! Default implementation of the KeyMint HAL and related HALs.
//!
//! This implementation of the HAL is only intended to allow testing and policy compliance.  A real
//! implementation **must implement the TA in a secure environment**, as per CDD 9.11 [C-1-1]:
//! "MUST back up the keystore implementation with an isolated execution environment."
//!
//! The additional device-specific components that are required for a real implementation of KeyMint
//! that is based on the Rust reference implementation are described in system/keymint/README.md.

use kmr_nonsecure_main::{local_keymint_setup, HalServiceError};
use log::{error, info, warn};

/// Name of KeyMint binder device instance.
static SERVICE_INSTANCE: &str = "default";

fn main() {
    if let Err(HalServiceError(e)) = inner_main() {
        panic!("HAL service failed: {:?}", e);
    }
}

fn inner_main() -> Result<(), HalServiceError> {
    // Initialize Android logging.
    android_logger::init_once(
        android_logger::Config::default()
            .with_tag("keymint-hal-nonsecure")
            .with_max_level(log::LevelFilter::Info)
            .with_log_buffer(android_logger::LogId::System),
    );
    // Redirect panic messages to logcat.
    std::panic::set_hook(Box::new(|panic_info| {
        error!("{}", panic_info);
    }));

    warn!("Insecure KeyMint HAL service is starting.");

    info!("Starting thread pool now.");
    binder::ProcessState::start_thread_pool();

    // Set up the local TA and register HAL services.
    local_keymint_setup(SERVICE_INSTANCE)?;

    binder::ProcessState::join_thread_pool();
    info!("KeyMint HAL service is terminating."); // should not reach here
    Ok(())
}
