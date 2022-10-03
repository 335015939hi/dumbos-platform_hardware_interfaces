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

#include "net_bluetooth_mgmt.h"

#define LOG_TAG "android.hardware.bluetooth.service.default"

#include <errno.h>
#include <fcntl.h>
#include <log/log.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "net_bluetooth_mgmt.h"

namespace aidl::android::hardware::bluetooth::impl {

int NetBluetoothMgmt::getBtHciSocket() {
  int fd = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
  if (fd < 0) {
    ALOGE("Bluetooth socket error: %s", strerror(errno));
    return -1;
  }
  return fd;
}

int NetBluetoothMgmt::bindBtHciSocket(int hci_interface, int fd) {
  if (waitHciDev(hci_interface)) {
    ALOGE("HCI interface (%d) not found", hci_interface);
    ::close(fd);
    return -1;
  }
  struct sockaddr_hci addr;
  memset(&addr, 0, sizeof(addr));
  addr.hci_family = AF_BLUETOOTH;
  addr.hci_dev = hci_interface;
  addr.hci_channel = HCI_CHANNEL_USER;
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    ALOGE("HCI Channel Control: %s", strerror(errno));
    ::close(fd);
    return -1;
  }
  ALOGI("HCI device ready");
  return fd;
}

int NetBluetoothMgmt::waitHciDev(int hci_interface) {
  struct sockaddr_hci addr;
  struct pollfd fds[1];
  struct mgmt_pkt ev;
  int fd;
  int ret = 0;

  ALOGI("%s", __func__);
  fd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
  if (fd < 0) {
    ALOGE("Bluetooth socket error: %s", strerror(errno));
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.hci_family = AF_BLUETOOTH;
  addr.hci_dev = HCI_DEV_NONE;
  addr.hci_channel = HCI_CHANNEL_CONTROL;
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    ALOGE("HCI Channel Control: %s", strerror(errno));
    ret = -1;
    goto end;
  }

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  /* Read Controller Index List Command */
  ev.opcode = MGMT_OP_INDEX_LIST;
  ev.index = HCI_DEV_NONE;
  ev.len = 0;

  ssize_t wrote;
  WRITE_NO_INTR(wrote = write(fd, &ev, 6));
  if (wrote != 6) {
    ALOGE("Unable to write mgmt command: %s", strerror(errno));
    ret = -1;
    goto end;
  }
  /* validate mentioned hci interface is present and registered with sock system
   */
  while (1) {
    int n;
    WRITE_NO_INTR(n = poll(fds, 1, -1));
    if (n == -1) {
      ALOGE("Poll error: %s", strerror(errno));
      ret = -1;
      break;
    } else if (n == 0) {
      ALOGE("Timeout, no HCI device detected");
      ret = -1;
      break;
    }

    if (fds[0].revents & POLLIN) {
      WRITE_NO_INTR(n = read(fd, &ev, sizeof(struct mgmt_pkt)));
      if (n < 0) {
        ALOGE("Error reading control channel: %s", strerror(errno));
        ret = -1;
        break;
      }

      if (ev.opcode == MGMT_EV_INDEX_ADDED && ev.index == hci_interface) {
        goto end;
      } else if (ev.opcode == MGMT_EV_COMMAND_COMP) {
        struct mgmt_event_read_index* cc;
        int i;

        cc = (struct mgmt_event_read_index*)ev.data;

        if (cc->cc_opcode != MGMT_OP_INDEX_LIST || cc->status != 0) continue;

        for (i = 0; i < cc->num_intf; i++) {
          if (cc->index[i] == hci_interface) goto end;
        }
      }
    }
  }

end:
  ::close(fd);
  return ret;
}

int NetBluetoothMgmt::findRfKill() {
  char rfkill_type[64];
  char type[16];
  int fd, size, i;
  for (i = 0; rfkill_state_path_ == nullptr && i < kMaxRfkillInterfaces; i++) {
    snprintf(rfkill_type, sizeof(rfkill_type),
             "/sys/class/rfkill/rfkill%d/type", i);
    if ((fd = open(rfkill_type, O_RDONLY)) < 0) {
      ALOGE("open(%s) failed: %s (%d)\n", rfkill_type, strerror(errno), errno);
      return -1;
    }

    size = read(fd, &type, sizeof(type));
    ::close(fd);

    if ((size >= 9) && !memcmp(type, "bluetooth", 9)) {
      ::asprintf(&rfkill_state_path_, "/sys/class/rfkill/rfkill%d/state", i);
      return 0;
    }
  }
  return 0;
}

int NetBluetoothMgmt::rfKill(int block) {
  int fd;
  char on = (block) ? '1' : '0';
  if (findRfKill() == -1) return 0;

  fd = open(rfkill_state_path_, O_WRONLY);
  if (fd < 0) {
    ALOGE("Unable to open %s", rfkill_state_path_);
    return -1;
  }
  ssize_t len;
  WRITE_NO_INTR(len = write(fd, &on, 1));
  if (len < 0) {
    ALOGE("Failed to change rfkill state");
    ::close(fd);
    return -1;
  }
  ::close(fd);
  return 0;
}

int NetBluetoothMgmt::openHci(int hci_interface) {
  ALOGI("%s", __func__);

  rfKill(1);

  int fd = getBtHciSocket();

  bt_soc_fd_ = fd;

  return bindBtHciSocket(hci_interface, fd);
}

void NetBluetoothMgmt::closeHci() {
  if (bt_soc_fd_ != -1) {
    ::close(bt_soc_fd_);
    bt_soc_fd_ = -1;
  }
  rfKill(0);
}

}  // namespace aidl::android::hardware::bluetooth::impl
