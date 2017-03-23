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
    struct iovec iov[2];
    ssize_t ret = 0;
#ifdef BT_DUMP_PKT
    ALOGD("%s: cmd type:%hhx, length:%zx", __func__, type, length);
    size_t i = 0;
    while (i < length) {
        ALOGD("%s:data[%zu]:0x%hhx", __func__, i, data[i]);
        i++;
    }
#endif //BT_DUMP_PKT
    iov[0].iov_base = &type;
    iov[0].iov_len = sizeof(type);
    iov[1].iov_base = (void *)data;
    iov[1].iov_len = length;
    while (1) {
        ret = TEMP_FAILURE_RETRY(writev(fd, iov, 2));
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

}  // namespace hci
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
