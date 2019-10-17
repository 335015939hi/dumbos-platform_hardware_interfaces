/*
 * Copyright (C) 2019 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_BOOTLOADERMESSAGE_V1_0_BOOTLOADERMESSAGE_H
#define ANDROID_HARDWARE_BOOTLOADERMESSAGE_V1_0_BOOTLOADERMESSAGE_H

#include <android/hardware/bootloader_message/1.0/IBootloaderMessage.h>

#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

#include <string>

namespace android {
namespace hardware {
namespace bootloader_message {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::bootloader_message::V1_0::IBootloaderMessage;
using ::android::hardware::bootloader_message::V1_0::Message;

/* Bootloader Message (2-KiB)
 *
 * This structure describes the content of a block in flash
 * that is used for recovery and the bootloader to talk to
 * each other.
 *
 * The command field is updated by linux when it wants to
 * reboot into recovery or to update radio or bootloader firmware.
 * It is also updated by the bootloader when firmware update
 * is complete (to boot into recovery for any final cleanup)
 *
 * The status field was used by the bootloader after the completion
 * of an "update-radio" or "update-hboot" command, which has been
 * deprecated since Froyo.
 *
 * The recovery field is only written by linux and used
 * for the system to send a message to recovery or the
 * other way around.
 *
 * The stage field is written by packages which restart themselves
 * multiple times, so that the UI can reflect which invocation of the
 * package it is.  If the value is of the format "#/#" (eg, "1/3"),
 * the UI will add a simple indicator of that status.
 *
 * We used to have slot_suffix field for A/B boot control metadata in
 * this struct, which gets unintentionally cleared by recovery or
 * uncrypt. Move it into struct bootloader_message_ab to avoid the
 * issue.
 */
struct MessageImpl {
    char command[32];
    char status[32];
    char recovery[768];

    // The 'recovery' field used to be 1024 bytes.  It has only ever
    // been used to store the recovery command line, so 768 bytes
    // should be plenty.  We carve off the last 256 bytes to store the
    // stage string (for multistage packages) and possible future
    // expansion.
    char stage[32];

    // The 'reserved' field used to be 224 bytes when it was initially
    // carved off from the 1024-byte recovery field. Bump it up to
    // 1184-byte so that the entire bootloader_message struct rounds up
    // to 2048-byte.
    char reserved[1184];
};

struct BootloaderMessage : public IBootloaderMessage {
  public:
    BootloaderMessage();
    explicit BootloaderMessage(std::string misc_block_device);

    Return<void> ReadBootloaderMessage(ReadBootloaderMessage_cb _hidl_cb) override;
    Return<void> WriteBootloaderMessage(const Message& message,
                                        WriteBootloaderMessage_cb _hidl_cb) override;
    Return<void> UpdateBootloaderMessageWithRecoveryOptions(
            const hidl_vec<hidl_string>& options, bool clear_fields,
            UpdateBootloaderMessageWithRecoveryOptions_cb _hidl_cb) override;
    Return<void> ReadWipePackage(uint32_t package_size, ReadWipePackage_cb _hidl_cb) override;
    Return<void> WriteWipePackage(const hidl_vec<uint8_t>& package_data,
                                  WriteWipePackage_cb _hidl_cb) override;

  private:
    std::string misc_block_device_;

    std::string GetMiscBlockDevice(std::string* err);

    bool WaitForDevice(const std::string& blk_device, std::string* err);

    bool ReadMiscPartition(void* p, uint32_t size, uint32_t offset, std::string* err);

    bool WriteMiscPartition(const void* p, uint32_t size, uint32_t offset, std::string* err);

    bool UpdateMessageStruct(Message* message, const hidl_vec<hidl_string>& recovery_options);
};

extern "C" BootloaderMessage* HIDL_FETCH_IBootloaderMessage(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bootloader_message
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BOOTLOADERMESSAGE_V1_0_BOOTLOADERMESSAGE_H
