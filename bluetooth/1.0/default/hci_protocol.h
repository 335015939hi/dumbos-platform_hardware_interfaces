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

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_vec;
using PacketReadCallback =
    std::function<void(HciPacketType, const hidl_vec<uint8_t> &)>;

// Implementation of HCI protocol bits common to different transports
class HciProtocol {
 public:
  virtual ~HciProtocol() {};

  // Protocol specific implementation of sending data
  virtual size_t Send(uint8_t type, const uint8_t *data, size_t length) = 0;
  // Hack to deal with internal commands sent by the vendor lib
  void SetWaitingInternalCommand(uint16_t opcode, tINT_CMD_CBACK callback);
  // Try to start the protocol. Returns |true| if success.
  virtual bool Start() = 0;

 protected:
  HciProtocol(PacketReadCallback packet_read_cb, async::AsyncFdWatcher *fd_watcher);

  enum ParseState { HCI_IDLE, HCI_PREAMBLE, HCI_PAYLOAD };

  // Called by implementors to indicate data of a particular type is ready.
  // Returns the state the particular stream was left in.
  // Will always return after finishing a packet.
  ParseState OnDataReady(int fd, HciPacketType type);

  static size_t WriteSafely(int fd, const uint8_t* data, size_t length);

  async::AsyncFdWatcher *fd_watcher_;
 private:
  PacketReadCallback packet_read_cb_;

  // Stores information for parsing an individual stream
  class ParseStream {
   public:
    ParseState state_;
    uint8_t preamble_[HCI_PREAMBLE_SIZE_MAX];
    hidl_vec<uint8_t> packet_;
    size_t bytes_remaining_;
    size_t bytes_read_;
  };

  ParseStream streams_[(HCI_PACKET_TYPE_EVENT - HCI_PACKET_TYPE_ACL_DATA) + 1] {};

  tINT_CMD_CBACK internal_command_cb_;
  uint16_t internal_command_opcode_;

  bool EventMatchesInternalCommand(const hidl_vec<uint8_t>& packet);
  HC_BT_HDR* WrapPacketAndCopy(uint16_t event, const hidl_vec<uint8_t>& data);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
