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

#include "hci_protocol.h"
#define LOG_TAG "android.hardware.bluetooth-hci-hci_protocol"
#include <android-base/logging.h>
#include <assert.h>
#include <fcntl.h>
#include <utils/Log.h>

#define HCI_CMD_HDR_LEN 3 /* sizeof(hci_command_hdr) */
namespace android {
namespace hardware {
namespace bluetooth {
namespace hci {
/*
 * defined under include/net/bluetooth/hci.h
 */
typedef struct {
    uint16_t    opcode;
    uint8_t     plen;
} __attribute__ ((packed))  hci_command_hdr;

size_t HciProtocol::WritevSafely(int fd, uint8_t type, const uint8_t* data, size_t length) {
  struct iovec iov[3];
  ssize_t ret = 0;
  int w_size = 0;
#ifdef BT_DUMP_PKT
  ALOGD("%s: cmd type:%hhx, length:%zx", __func__, type, length);
  size_t i = 0;
  while (i < length) {
        ALOGD("%s:data[%zu]:0x%hhx", __func__, i, data[i]);
        i++;
  }
#endif //BT_DUMP_PKT
  hci_command_hdr hc_pkt;
  if (type == HCI_PACKET_TYPE_COMMAND) {
      hc_pkt.opcode = data[0] | (data[1] << 8);
      hc_pkt.plen = length - sizeof(hci_command_hdr);

      iov[0].iov_base = &type;
      iov[0].iov_len = sizeof(type);
      iov[1].iov_base = &hc_pkt;
      iov[1].iov_len = HCI_CMD_HDR_LEN;
      w_size = 2;

      if (hc_pkt.plen) {
          /* more parameter data in hci command */
          iov[2].iov_base = (void *) (data + HCI_CMD_HDR_LEN);
          iov[2].iov_len = hc_pkt.plen;
          w_size = 3;
      }
  } else {
      iov[0].iov_base = &type;
      iov[0].iov_len = sizeof(type);
      iov[1].iov_base = (void *)data;
      iov[1].iov_len = length;
      w_size = 2;
  }
  while (1) {
    ret = TEMP_FAILURE_RETRY(writev(fd, iov, w_size));
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
    ALOGE("%s Writing again length:%zx,tranmitted ret:%zx, ret:%zx", __func__, length, transmitted_length, ret);
  }

  return transmitted_length;
}

}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
