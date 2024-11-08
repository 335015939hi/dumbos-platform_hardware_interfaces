/**
 * Copyright 2024, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <stdint.h>

/**
 * Callabcks from C to Rust
 */

enum Status {
  STATUS_SUCCESS,
  STATUS_ALREADY_INITIALIZED,
  STATUS_UNABLE_TO_OPEN_INTERFACE,
  STATUS_HARDWARE_INITIALIZATION_ERROR,
  STATUS_UNKNOWN,
};

struct callbacks {
  void* handle;
  void (*initialization_complete)(const void* handle, enum Status);
  void (*recv_event)(const void* handle, const uint8_t* data, size_t len);
  void (*recv_acl)(const void* handle, const uint8_t* data, size_t len);
  void (*recv_sco)(const void* handle, const uint8_t* data, size_t len);
  void (*recv_iso)(const void* handle, const uint8_t* data, size_t len);
};

/**
 * Interface from Rust to C
 */

struct hal;

void hal_initialize(struct callbacks*, struct hal**);
void hal_close(struct hal*);
void hal_send_command(struct hal*, const uint8_t*, size_t);
void hal_send_acl(struct hal*, const uint8_t*, size_t);
void hal_send_sco(struct hal*, const uint8_t*, size_t);
void hal_send_iso(struct hal*, const uint8_t*, size_t);
