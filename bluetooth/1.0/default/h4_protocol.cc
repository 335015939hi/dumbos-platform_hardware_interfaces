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

#include "h4_protocol.h"

#include <assert.h>

#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <android-base/logging.h>
#include <utils/Log.h>

#include <fcntl.h>

static const int INVALID_FD = -1;

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

H4Protocol::H4Protocol(
    int fd,
    PacketReadCallback packet_read_cb,
    async::AsyncFdWatcher *fd_watcher)
    : HciProtocol(packet_read_cb, fd_watcher) {
  uart_fd_ = fd;
}

bool H4Protocol::Start() {
  if (uart_fd_ == INVALID_FD) {
    ALOGE("%s unable to determine UART fd", __func__);
    return false;
  }

  ALOGI("%s UART fd: %d", __func__, uart_fd_);

  fd_watcher_->WatchFdForNonBlockingReads(
      uart_fd_,
      [this](int fd) { OnDataReady(fd); }
  );

  return true;
}

size_t H4Protocol::Send(uint8_t type, const uint8_t *data, size_t length) {
  if (uart_fd_ == INVALID_FD) {
    return 0;
  }

  int rv = WriteSafely(uart_fd_, &type, sizeof(type));
  if (rv == sizeof(type)) {
    rv = WriteSafely(uart_fd_, data, length);
  }

  return rv;
}

void H4Protocol::OnDataReady(int fd) {
  if (!stream_has_interpretation_) {
    uint8_t buffer[1] = {0};
    size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, buffer, 1));
    CHECK(bytes_read == 1);
    hci_packet_type_ = static_cast<HciPacketType>(buffer[0]);
  }

  ParseState end_state = HciProtocol::OnDataReady(fd, hci_packet_type_);
  stream_has_interpretation_ = end_state != HCI_IDLE;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
