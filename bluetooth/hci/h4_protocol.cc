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
                       PacketReadCallback evt_cb, PacketReadCallback iso_cb,
                       DisconnectCallback disconnect_cb)
    : uart_fd_(fd),
      disconnect_cb_(std::move(disconnect_cb)),
      h4_parser_([
        cmd_cb = std::move(cmd_cb),
        acl_cb = std::move(acl_cb),
        sco_cb = std::move(sco_cb),
        evt_cb = std::move(evt_cb),
        iso_cb = std::move(iso_cb)
      ] (H4Parser::Idc idc, std::vector<uint8_t> const& packet) {
        switch (idc) {
          case H4Parser::Idc::kCommand: cmd_cb(packet); break;
          case H4Parser::Idc::kAcl: acl_cb(packet); break;
          case H4Parser::Idc::kSco: sco_cb(packet); break;
          case H4Parser::Idc::kEvent: evt_cb(packet); break;
          case H4Parser::Idc::kIso: iso_cb(packet); break;
          case H4Parser::Idc::kUnknown:
          default: break;
        }
      }) {}

size_t H4Protocol::Send(H4Parser::Idc type, const std::vector<uint8_t>& vector) {
  return Send(type, vector.data(), vector.size());
}

size_t H4Protocol::Send(H4Parser::Idc type, const uint8_t* data, size_t length) {
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
      LOG_ALWAYS_FATAL("%s error writing to UART (%s)", __func__,
                       strerror(errno));
    } else if (ret == 0) {
      // Nothing written :(
      ALOGE("%s zero bytes written - something went wrong...", __func__);
      break;
    }
    break;
  }
  return ret;
}

void H4Protocol::SendDataToPacketizer(uint8_t const* buffer, size_t length) {
  h4_parser_.Consume(buffer, length);
}

void H4Protocol::OnDataReady() {
  if (disconnected_) {
    return;
  }
  uint8_t buffer[kMaxPacketLength];
  ssize_t bytes_read =
      TEMP_FAILURE_RETRY(read(uart_fd_, buffer, kMaxPacketLength));
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
  SendDataToPacketizer(buffer, bytes_read);
}

}  // namespace android::hardware::bluetooth::hci
