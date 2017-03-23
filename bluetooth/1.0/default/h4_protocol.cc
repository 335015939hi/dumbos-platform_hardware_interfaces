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

#include "h4_protocol.h"

#define LOG_TAG "android.hardware.bluetooth-hci-h4"
#include <android-base/logging.h>
#include <assert.h>
#include <fcntl.h>
#include <utils/Log.h>

namespace android {
namespace hardware {
namespace bluetooth {
namespace hci {

size_t H4Protocol::Send(uint8_t type, const uint8_t* data, size_t length) {
    int rv;
#if BT_USB
    /* For HCI communication over USB dongle, multiple write results in
     * response timeout as driver expect type + data at once to process
     * the command, so using "writev"(for atomicity) here.
     */
    rv = WritevSafely(uart_fd_, type, data, length);
#else
    rv = WriteSafely(uart_fd_, &type, sizeof(type));
    if (rv == sizeof(type)) {
        rv = WriteSafely(uart_fd_, data, length);
    }
#endif
    return rv;
}

void H4Protocol::OnPacketReady() {
  switch (hci_packet_type_) {
    case HCI_PACKET_TYPE_EVENT:
      event_cb_(hci_packetizer_.GetPacket());
      break;
    case HCI_PACKET_TYPE_ACL_DATA:
      acl_cb_(hci_packetizer_.GetPacket());
      break;
    case HCI_PACKET_TYPE_SCO_DATA:
      sco_cb_(hci_packetizer_.GetPacket());
      break;
    default: {
      bool bad_packet_type = true;
      CHECK(!bad_packet_type);
    }
  }
  // Get ready for the next type byte.
  hci_packet_type_ = HCI_PACKET_TYPE_UNKNOWN;
}
#if BT_USB
void H4Protocol::OnDataReady(int fd) {
  if (hci_packet_type_ == HCI_PACKET_TYPE_UNKNOWN) {
    /**
      * read full buffer. 512 is just arbitrary number. What is max response
      * length of bt packet ? - Couldn't locate from BT spec v4.2 Suggestion ??
      * Question : Why to read in single chunk rather than multiple reads,
      * which can give parameter length arriving in response ?
      * Answer: The multiple reads does not work with BT USB dongle. At least
      * with Bluetooth 2.0 supported USB dongle. After first read, either
      * firmware/kernel (do not know who is responsible - inputs ??) driver
      * discard the whole message and successive read results in forever
      * blocking loop. - Is there any other way to make it work with multiple
      * reads, do not know yet (it can eliminate need of this function) ?
      * Reading in single shot gives expected response.
      */
    const size_t buf_size = 512;
    uint8_t buf[buf_size];
    ssize_t len = read(fd, buf, buf_size);
    if (len <= 0) {
        ALOGE("Read error, Unexpected length:%s", strerror(errno));
        return;
    }
    hci_packet_type_ = static_cast<HciPacketType>(buf[0]);
    ALOGD("%s: hci pkt type:%x, read len:%zx", __func__, hci_packet_type_, len);
    /* TODO: validate packet type */
    hci_packetizer_.CbHciPacket(hci_packet_type_, buf+1, len-1);
  }
}
#else
void H4Protocol::OnDataReady(int fd) {
  if (hci_packet_type_ == HCI_PACKET_TYPE_UNKNOWN) {
    uint8_t buffer[1] = {0};
    size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, buffer, 1));
    CHECK(bytes_read == 1);
    hci_packet_type_ = static_cast<HciPacketType>(buffer[0]);
  } else {
    hci_packetizer_.OnDataReady(fd, hci_packet_type_);
  }
}
#endif //BT_USB


}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
