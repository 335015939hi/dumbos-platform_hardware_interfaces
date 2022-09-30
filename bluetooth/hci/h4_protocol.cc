/*
 * Copyright 2022 The Android Open Source Project
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

#include "h4_protocol.h"

#define LOG_TAG "android.hardware.bluetooth.hci-h4"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/uio.h>

#include "log/log.h"

namespace android::hardware::bluetooth::hci {

H4Protocol::H4Protocol(int fd, PacketReadCallback cmd_cb,
                       PacketReadCallback acl_cb, PacketReadCallback sco_cb,
                       PacketReadCallback event_cb, PacketReadCallback iso_cb,
                       DisconnectCallback disconnect_cb)
    : uart_fd_(fd),
      cmd_cb_(cmd_cb),
      acl_cb_(acl_cb),
      sco_cb_(sco_cb),
      event_cb_(event_cb),
      iso_cb_(iso_cb),
      disconnect_cb_(disconnect_cb) {}

size_t H4Protocol::Send(PacketType type, const std::vector<uint8_t>& vector) {
  return Send(type, vector.data(), vector.size());
}

size_t H4Protocol::Send(PacketType type, const uint8_t* data, size_t length) {
  /* For HCI communication over USB dongle, multiple write results in
   * response timeout as driver expect type + data at once to process
   * the command, so using "writev"(for atomicity) here.
   */
  struct iovec iov[2];
  ssize_t ret = 0;
  iov[0].iov_base = &type;
  iov[0].iov_len = sizeof(type);
  iov[1].iov_base = (void*)data;
  iov[1].iov_len = length;
  while (1) {
    ret = TEMP_FAILURE_RETRY(writev(uart_fd_, iov, 2));
    if (ret == -1) {
      if (errno == EAGAIN) {
        ALOGE("%s error writing to UART (%s)", __func__, strerror(errno));
        continue;
      }
    } else if (ret == 0) {
      // Nothing written :(
      ALOGE("%s zero bytes written - something went wrong...", __func__);
      break;
    }
    break;
  }
  return ret;
}

void H4Protocol::OnPacketReady(const std::vector<uint8_t>& packet) {
  switch (hci_packet_type_) {
    case PacketType::COMMAND:
      cmd_cb_(packet);
      break;
    case PacketType::ACL_DATA:
      acl_cb_(packet);
      break;
    case PacketType::SCO_DATA:
      sco_cb_(packet);
      break;
    case PacketType::EVENT:
      event_cb_(packet);
      break;
    case PacketType::ISO_DATA:
      iso_cb_(packet);
      break;
    default: {
      LOG_ALWAYS_FATAL("Bad packet type 0x%x",
                       static_cast<int>(hci_packet_type_));
    }
  }
  // Get ready for the next type byte.
  hci_packet_type_ = PacketType::UNKNOWN;
  buffer_offset_ += packet.size();
  SendDataToPacketizer();
}

void H4Protocol::SendDataToPacketizer() {
  if (hci_packet_type_ == PacketType::UNKNOWN) {
    if (buffer_offset_ < packet_buffer_.size()) {
      hci_packet_type_ =
          static_cast<PacketType>(packet_buffer_.data()[buffer_offset_]);
      buffer_offset_ += 1;
    }
  }
  if (buffer_offset_ < packet_buffer_.size() - 1) {
    bool packet_ready = hci_packetizer_.OnDataReady(
        hci_packet_type_, packet_buffer_, buffer_offset_);
    if (packet_ready) {
      OnPacketReady(hci_packetizer_.GetPacket());
    }
  }
  // All of the bytes have been consumed, so reset the buffer.
  buffer_offset_ = 0;
}

void H4Protocol::OnDataReady() {
  if (disconnected_) {
    return;
  }
  packet_buffer_.resize(kMaxPacketLength);
  ssize_t bytes_read = TEMP_FAILURE_RETRY(
      read(uart_fd_, packet_buffer_.data(), kMaxPacketLength));
  if (bytes_read == 0) {
    ALOGI("No bytes read, calling the disconnect callback");
    disconnected_ = true;
    disconnect_cb_();
    return;
  }
  if (bytes_read < 0) {
    ALOGW("error reading from UART (%s)", strerror(errno));
    return;
  }
  packet_buffer_.resize(bytes_read);
  SendDataToPacketizer();
}

}  // namespace android::hardware::bluetooth::hci
