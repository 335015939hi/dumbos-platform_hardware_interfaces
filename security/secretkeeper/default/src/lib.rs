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

//! Common functionality for non-secure/testing instance of Secretkeeper.
use secretkeeper_comm::data_types::error::Error;
use secretkeeper_core::KeyValueStore;
use std::collections::HashMap;
use std::sync::Arc;
use std::sync::Mutex;

/// An in-memory implementation of KeyValueStore. Please note that this is entirely for
/// testing purposes. Refer to the documentation of `SecretkeeperStore` & Secretkeeper HAL for
/// persistence requirements.
pub struct InMemoryStore(Arc<Mutex<HashMap<Vec<u8>, Vec<u8>>>>);

impl InMemoryStore {
    /// Constructor of a new & empty instance.
    pub fn new() -> Self {
        Self(Arc::new(Mutex::new(HashMap::new())))
    }
}

impl KeyValueStore for InMemoryStore {
    fn store(&self, key: Vec<u8>, val: Vec<u8>) -> Result<(), Error> {
        // This will overwrite the value if key is already present.
        let _ = self.0.lock().unwrap().insert(key, val);
        Ok(())
    }

    fn get(&self, key: &[u8]) -> Result<Option<Vec<u8>>, Error> {
        let db = self.0.lock().unwrap();
        let optional_val = db.get(key);
        Ok(optional_val.cloned())
    }
}
