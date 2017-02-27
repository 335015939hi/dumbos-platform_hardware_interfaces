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

#include "mct_protocol.h"

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

MctProtocol::MctProtocol(
    int *fds,
    PacketReadCallback packet_read_cb,
    async::AsyncFdWatcher *fd_watcher)
    : HciProtocol(packet_read_cb, fd_watcher) {
  for (int i = 0; i < CH_MAX; i++) {
    uart_fds_[i] = fds[i];
  }
}

bool MctProtocol::Start() {
  ALOGI(
      "%s UART fds: CMD=%d, EVT=%d, ACL_OUT=%d, ACL_IN=%d",
      __func__,
      uart_fds_[CH_CMD],
      uart_fds_[CH_EVT],
      uart_fds_[CH_ACL_OUT],
      uart_fds_[CH_ACL_IN]
  );

  CHECK(uart_fds_[CH_CMD] != INVALID_FD);
  CHECK(uart_fds_[CH_EVT] != INVALID_FD);
  CHECK(uart_fds_[CH_ACL_OUT] != INVALID_FD);
  CHECK(uart_fds_[CH_ACL_IN] != INVALID_FD);

  fd_watcher_->WatchFdForNonBlockingReads(
      uart_fds_[CH_EVT],
      [this](int fd) { OnEventReady(fd); }
  );

  fd_watcher_->WatchFdForNonBlockingReads(
      uart_fds_[CH_ACL_IN],
      [this](int fd) { OnAclDataReady(fd); }
  );

  return true;
}

size_t MctProtocol::Send(uint8_t type, const uint8_t *data, size_t length) {
  if (type == HCI_PACKET_TYPE_ACL_DATA) {
    return WriteSafely(uart_fds_[CH_ACL_OUT], data, length);
  } else if (type == HCI_PACKET_TYPE_COMMAND) {
    return WriteSafely(uart_fds_[CH_CMD], data, length);
  }

  ALOGE("%s invalid data type: %d", __func__, type);
  return 0;
}

void MctProtocol::OnAclDataReady(int fd) {
  HciProtocol::OnDataReady(fd, HCI_PACKET_TYPE_ACL_DATA);
}

void MctProtocol::OnEventReady(int fd) {
  HciProtocol::OnDataReady(fd, HCI_PACKET_TYPE_EVENT);
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
