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

//! Local in-process implementation of the KeyMint TA. This is insecure and should
//! only be used for testing purposes.

// This crate is `std` using, but some of the code uses macros from a `no_std` world.
extern crate alloc;

use gk_boringssl as boring;
use gk_ta::{traits, Error, FailureRecord};
use gk_wire::{AndroidUserId, MillisecondsSinceEpoch};
use log::{debug, error, info, warn};

/// Build a [`gk_ta::GatekeeperTa`] instance for nonsecure use.
pub fn build_ta() -> gk_ta::GatekeeperTa {
    info!("Building NON-SECURE Gatekeeper Rust TA");

    let rng = boring::Rng;
    let clock = StdClock::default();
    let auth_key = traits::ExplicitAuthKey::new(Box::new(boring::HmacSha256));

    // Store failure records in memory.  This is insecure and means that passwords will
    // not survive a reboot.
    let failures = FailureRecording::<40>::default();

    // Pre-shared key of all-zeros for `ISharedSecret` agreement, matching:
    // - `kFakeAgreementKey` in `system/keymaster/km_openssl/soft_keymaster_enforcement.cpp`
    // - `Keys::kak` in `hardware/interfaces/security/keymint/aidl/default/ta/soft.rs`
    const SS_PRESHARED_KEY: boring::PreSharedKey = [0; 32];
    let preshared_key = Box::new(boring::FixedPreSharedKey(SS_PRESHARED_KEY));
    let ss_derive = boring::SharedSecretDerive::new(preshared_key);

    let imp = traits::Implementation {
        rng: Box::new(rng),
        clock: Box::new(clock),
        compare: Box::new(boring::ConstEq),
        hmac: Box::new(boring::HmacSha256),
        password: Box::new(NonsecurePasswordKey),
        auth_key: Box::new(auth_key),
        failures: Box::new(failures),
        shared_secret: Some(Box::new(ss_derive)),
    };
    gk_ta::GatekeeperTa::new(imp)
}

/// Monotonic clock.
#[derive(Default)]
pub struct StdClock;

impl traits::MonotonicClock for StdClock {
    fn now(&self) -> MillisecondsSinceEpoch {
        let mut time = libc::timespec {
            tv_sec: 0,
            tv_nsec: 0,
        };
        // Use `CLOCK_BOOTTIME` for consistency with the times used by the Cuttlefish
        // C++ implementation of Gatekeeper.
        let rc =
        // Safety: `time` is a valid structure.
            unsafe { libc::clock_gettime(libc::CLOCK_BOOTTIME, &mut time as *mut libc::timespec) };
        if rc < 0 {
            log::warn!("failed to get time!");
            return MillisecondsSinceEpoch(0);
        }
        MillisecondsSinceEpoch(((time.tv_sec * 1000) + (time.tv_nsec / 1000 / 1000)).into())
    }
}

/// Fake password key.
struct NonsecurePasswordKey;

impl traits::PasswordKeyRetrieval for NonsecurePasswordKey {
    fn key(&self) -> Result<traits::OpaqueOr<traits::HmacKey>, gk_ta::Error> {
        let fake_key = vec![0; 32];
        Ok(traits::OpaqueOr::Explicit(traits::HmacKey(fake_key)))
    }
}

type MemFailureRecord = Option<(AndroidUserId, FailureRecord)>;

/// In-memory implementation of failure recording, with a fixed number of records;
pub struct FailureRecording<const N: usize> {
    records: [MemFailureRecord; N],
}

impl<const N: usize> Default for FailureRecording<N> {
    fn default() -> Self {
        Self {
            records: core::array::from_fn(|_| None),
        }
    }
}

impl<const N: usize> traits::FailureRecording for FailureRecording<N> {
    fn get(&self, user_id: AndroidUserId) -> Result<Option<FailureRecord>, Error> {
        Ok(self
            .records
            .iter()
            .filter_map(|m| m.as_ref())
            .find_map(|(uid, record)| {
                if *uid == user_id {
                    Some(record.clone())
                } else {
                    None
                }
            }))
    }

    fn set(&mut self, user_id: AndroidUserId, record: &FailureRecord) -> Result<(), Error> {
        if let Some(existing_idx) = self
            .records
            .iter()
            .position(|m| matches!(m, Some((uid, _)) if *uid == user_id))
        {
            debug!("replacing existing failure record for {user_id:?}");
            self.records[existing_idx] = Some((user_id, record.clone()));
            return Ok(());
        }

        // Look for an empty slot, but also calculate the oldest existing record along the way.
        let mut min_idx = None;
        let mut min_timestamp = MillisecondsSinceEpoch(i64::MAX);
        for (idx, mem_record) in self.records.iter().enumerate() {
            match mem_record {
                Some((uid, _record)) if *uid == user_id => {
                    // This should have been found above!
                    error!("unexpected revord for {user_id:?}");
                    return Err(Error::Internal);
                }
                Some((_uid, record)) => {
                    if record.last_checked_timestamp <= min_timestamp {
                        min_idx = Some(idx);
                        min_timestamp = record.last_checked_timestamp;
                    }
                }
                None => {
                    debug!("new failure record for {user_id:?}");
                    self.records[idx] = Some((user_id, record.clone()));
                    return Ok(());
                }
            }
        }
        let Some(idx) = min_idx else {
            return Err(Error::Internal);
        };
        // TODO: check this behaviour vs C++ code in Trusty
        warn!("overwriting existing failure record with one for {user_id:?}");
        self.records[idx] = Some((user_id, record.clone()));
        Ok(())
    }

    fn clear(&mut self, user_id: AndroidUserId) -> Result<bool, Error> {
        for (idx, mem_record) in self.records.iter_mut().enumerate() {
            let Some((uid, _record)) = mem_record else {
                continue;
            };
            if *uid == user_id {
                info!("clearing in-memory record for {user_id:?} found");
                self.records[idx] = None;
                return Ok(true);
            }
        }
        info!("no in-memory record for {user_id:?} found");
        Ok(false)
    }

    fn clear_all(&mut self) -> Result<(), Error> {
        warn!("clearing all in-memory records");
        for idx in 0..N {
            self.records[idx] = None;
        }
        Ok(())
    }
}
