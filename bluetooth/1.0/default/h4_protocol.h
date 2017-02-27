//
// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <hidl/HidlSupport.h>

#include "async_fd_watcher.h"
#include "bt_vendor_lib.h"
#include "hci_internals.h"
#include "hci_protocol.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

class H4Protocol : public HciProtocol {
 public:
  H4Protocol(
      int fd,
      PacketReadCallback packet_read_cb,
      async::AsyncFdWatcher *fd_watcher);

  size_t Send(uint8_t type, const uint8_t *data, size_t length);

  bool Start();

 private:
  void OnDataReady(int fd);

  int uart_fd_;
  HciPacketType hci_packet_type_{HCI_PACKET_TYPE_UNKNOWN};
  bool stream_has_interpretation_;
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
