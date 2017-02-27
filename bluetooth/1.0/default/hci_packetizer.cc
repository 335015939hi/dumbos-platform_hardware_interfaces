//
// Copyright 2017 The Android Open Source Project
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

#include "hci_packetizer.h"

#define LOG_TAG "android.hardware.bluetooth.hci_packetizer"
#include <android-base/logging.h>
#include <utils/Log.h>

#include <dlfcn.h>
#include <fcntl.h>

namespace {

const size_t preamble_size_for_type[] = {
    0, HCI_COMMAND_PREAMBLE_SIZE, HCI_ACL_PREAMBLE_SIZE, HCI_SCO_PREAMBLE_SIZE,
    HCI_EVENT_PREAMBLE_SIZE};
const size_t packet_length_offset_for_type[] = {
    0, HCI_LENGTH_OFFSET_CMD, HCI_LENGTH_OFFSET_ACL, HCI_LENGTH_OFFSET_SCO,
    HCI_LENGTH_OFFSET_EVT};

size_t HciGetPacketLengthForType(HciPacketType type, const uint8_t* preamble) {
  size_t offset = packet_length_offset_for_type[type];
  if (type != HCI_PACKET_TYPE_ACL_DATA) return preamble[offset];
  return (((preamble[offset + 1]) << 8) | preamble[offset]);
}

}  // namespace

namespace android {
namespace hardware {
namespace bluetooth {
namespace hci {

const hidl_vec<uint8_t>& HciPacketizer::GetPacket() const {
  return hci_packet_;
}

void HciPacketizer::OnDataReady(int fd, HciPacketType hci_packet_type) {
  switch (hci_parser_state_) {
    case HCI_PREAMBLE: {
      size_t bytes_read = TEMP_FAILURE_RETRY(read(
          fd, hci_packet_preamble_ + hci_packet_bytes_read_,
          preamble_size_for_type[hci_packet_type] - hci_packet_bytes_read_));
      CHECK(bytes_read > 0);
      hci_packet_bytes_read_ += bytes_read;
      if (hci_packet_bytes_read_ == preamble_size_for_type[hci_packet_type]) {
        size_t packet_length =
            HciGetPacketLengthForType(hci_packet_type, hci_packet_preamble_);
        hci_packet_.resize(preamble_size_for_type[hci_packet_type] +
                           packet_length);
        memcpy(hci_packet_.data(), hci_packet_preamble_,
               preamble_size_for_type[hci_packet_type]);
        hci_packet_bytes_remaining_ = packet_length;
        hci_parser_state_ = HCI_PAYLOAD;
        hci_packet_bytes_read_ = 0;
      }
      break;
    }

    case HCI_PAYLOAD: {
      size_t bytes_read = TEMP_FAILURE_RETRY(read(
          fd, hci_packet_.data() + preamble_size_for_type[hci_packet_type] +
                  hci_packet_bytes_read_,
          hci_packet_bytes_remaining_));
      CHECK(bytes_read > 0);
      hci_packet_bytes_remaining_ -= bytes_read;
      hci_packet_bytes_read_ += bytes_read;
      if (hci_packet_bytes_remaining_ == 0) {
        hci_packet_ready_cb_();
        hci_parser_state_ = HCI_PREAMBLE;
        hci_packet_bytes_read_ = 0;
      }
      break;
    }
  }
}

}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
