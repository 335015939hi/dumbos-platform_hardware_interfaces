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

#include "hci_protocol.h"

#include <assert.h>

#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <android-base/logging.h>
#include <cutils/properties.h>
#include <utils/Log.h>

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

} // namespace

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

HciProtocol::HciProtocol(PacketReadCallback packet_read_cb, async::AsyncFdWatcher *fd_watcher) {
  packet_read_cb_ = packet_read_cb;
  fd_watcher_ = fd_watcher;
}

void HciProtocol::SetWaitingInternalCommand(uint16_t opcode, tINT_CMD_CBACK callback) {
  internal_command_opcode_ = opcode;
  internal_command_cb_ = callback;
}

size_t HciProtocol::WriteSafely(int fd, const uint8_t* data, size_t length) {
  size_t transmitted_length = 0;
  while (length > 0) {
    ssize_t ret =
        TEMP_FAILURE_RETRY(write(fd, data + transmitted_length, length));

    if (ret == -1) {
      if (errno == EAGAIN) continue;
      ALOGE("%s error writing to UART (%s)", __func__, strerror(errno));
      break;

    } else if (ret == 0) {
      // Nothing written :(
      ALOGE("%s zero bytes written - something went wrong...", __func__);
      break;
    }

    transmitted_length += ret;
    length -= ret;
  }

  return transmitted_length;
}

HciProtocol::ParseState HciProtocol::OnDataReady(int fd, HciPacketType type) {
  CHECK(type >= HCI_PACKET_TYPE_ACL_DATA &&
        type <= HCI_PACKET_TYPE_EVENT) << "type = " << static_cast<unsigned int>(type);

  ParseStream *stream = &streams_[type - HCI_PACKET_TYPE_ACL_DATA];

  switch (stream->state_) {
    case HCI_IDLE: {
      // TODO(eisenbach): Check for workaround(s)
      stream->state_ = HCI_PREAMBLE;
      stream->bytes_remaining_ = preamble_size_for_type[type];
      stream->bytes_read_ = 0;
      break;
    }

    case HCI_PREAMBLE: {
      size_t bytes_read = TEMP_FAILURE_RETRY(
          read(fd, stream->preamble_ + stream->bytes_read_,
               stream->bytes_remaining_));
      CHECK(bytes_read > 0);
      stream->bytes_remaining_ -= bytes_read;
      stream->bytes_read_ += bytes_read;
      if (stream->bytes_remaining_ == 0) {
        size_t packet_length =
            HciGetPacketLengthForType(type, stream->preamble_);
        stream->packet_.resize(preamble_size_for_type[type] +
                           packet_length);
        memcpy(stream->packet_.data(), stream->preamble_,
               preamble_size_for_type[type]);
        stream->bytes_remaining_ = packet_length;
        stream->state_ = HCI_PAYLOAD;
        stream->bytes_read_ = 0;
      }
      break;
    }

    case HCI_PAYLOAD: {
      size_t bytes_read = TEMP_FAILURE_RETRY(
          read(fd,
               stream->packet_.data() + preamble_size_for_type[type] +
                   stream->bytes_read_,
               stream->bytes_remaining_));
      CHECK(bytes_read > 0);
      stream->bytes_remaining_ -= bytes_read;
      stream->bytes_read_ += bytes_read;
      if (stream->bytes_remaining_ == 0) {
        if (internal_command_cb_ != nullptr &&
            type == HCI_PACKET_TYPE_EVENT &&
            EventMatchesInternalCommand(stream->packet_)) {
          HC_BT_HDR* bt_hdr =
              WrapPacketAndCopy(HCI_PACKET_TYPE_EVENT, stream->packet_);

          // The callbacks can send new commands, so don't zero after calling.
          tINT_CMD_CBACK saved_cb = internal_command_cb_;
          internal_command_cb_ = nullptr;
          saved_cb(bt_hdr);
        } else {
          packet_read_cb_(type, stream->packet_);
        }
        stream->state_ = HCI_IDLE;
      }
      break;
    }
  }

  return stream->state_;
}

bool HciProtocol::EventMatchesInternalCommand(const hidl_vec<uint8_t>& packet) {
  uint8_t event_code = packet[0];
  if (event_code != HCI_COMMAND_COMPLETE_EVENT) {
    ALOGE("%s: Unhandled event type %02X", __func__, event_code);
    return false;
  }

  size_t opcode_offset = HCI_EVENT_PREAMBLE_SIZE + 1;  // Skip num packets.

  uint16_t opcode = packet[opcode_offset] | (packet[opcode_offset + 1] << 8);

  ALOGV("%s internal_command.opcode = %04X opcode = %04x", __func__,
        internal_command_opcode_, opcode);
  return opcode == internal_command_opcode_;
}

HC_BT_HDR* HciProtocol::WrapPacketAndCopy(uint16_t event, const hidl_vec<uint8_t>& data) {
  size_t packet_size = data.size() + sizeof(HC_BT_HDR);
  HC_BT_HDR* packet = reinterpret_cast<HC_BT_HDR*>(new uint8_t[packet_size]);
  packet->offset = 0;
  packet->len = data.size();
  packet->layer_specific = 0;
  packet->event = event;
  // TODO(eisenbach): Avoid copy here; if BT_HDR->data can be ensured to
  // be the only way the data is accessed, a pointer could be passed here...
  memcpy(packet->data, data.data(), data.size());
  return packet;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
