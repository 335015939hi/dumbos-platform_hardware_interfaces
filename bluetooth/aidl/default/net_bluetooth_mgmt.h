/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <sys/socket.h>

#include <cstdint>
#include <cstdlib>

#define BTPROTO_HCI 1

#define HCI_CHANNEL_USER 1
#define HCI_CHANNEL_CONTROL 3
#define HCI_DEV_NONE 0xffff

/* reference from <kernel>/include/net/bluetooth/mgmt.h */
#define MGMT_OP_INDEX_LIST 0x0003
#define MGMT_EV_INDEX_ADDED 0x0004
#define MGMT_EV_COMMAND_COMP 0x0001
#define MGMT_EV_SIZE_MAX 1024
#define WRITE_NO_INTR(fn) \
  do {                    \
  } while ((fn) == -1 && errno == EINTR)

struct sockaddr_hci {
  sa_family_t hci_family;
  unsigned short hci_dev;
  unsigned short hci_channel;
};

struct mgmt_pkt {
  uint16_t opcode;
  uint16_t index;
  uint16_t len;
  uint8_t data[MGMT_EV_SIZE_MAX];
} __attribute__((packed));

struct mgmt_event_read_index {
  uint16_t cc_opcode;
  uint8_t status;
  uint16_t num_intf;
  uint16_t index[0];
} __attribute__((packed));

namespace aidl::android::hardware::bluetooth::impl {

class NetBluetoothMgmt {
 public:
  NetBluetoothMgmt() {}

  ~NetBluetoothMgmt() {
    if (rfkill_state_path_ != nullptr) {
      free(rfkill_state_path_);
    }
  }

  int openHci(int hci_interface = 0);
  void closeHci();

  static constexpr int kMaxRfkillInterfaces = 20;

 private:
  int findRfKill();
  int waitHciDev(int hci_interface);
  int getBtHciSocket();
  int bindBtHciSocket(int hci_interface, int fd);
  int rfKill(int block);

  int bt_soc_fd_{-1};
  char* rfkill_state_path_{nullptr};
};

}  // namespace aidl::android::hardware::bluetooth::impl