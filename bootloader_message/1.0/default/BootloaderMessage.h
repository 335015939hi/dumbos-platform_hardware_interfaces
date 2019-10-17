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
 * This implementation of the Message struct is in line with the definition in the legacy
 * libbootloader_message. We use fixed length char arrays to store each message field. And the size
 * of the struct adds up to 2 KiB.
 */
struct MessageImpl {
    char command[32];
    char status[32];
    // The 'recovery' field used to be 1024 bytes.  It has only ever been used to store the
    // recovery command line, so 768 bytes should be plenty.  We carve off the last 256 bytes to
    // store the stage string (for multistage packages) and possible future expansion.
    char recovery[768];
    char stage[32];

    // The 'reserved' field used to be 224 bytes when it was initially
    // carved off from the 1024-byte recovery field. Bump it up to
    // 1184-byte so that the entire bootloader_message struct rounds up
    // to 2048-byte.
    char reserved[1184];
};

struct BootloaderMessage : public IBootloaderMessage {
  public:
    BootloaderMessage() = default;
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
    // The cached path to the block device that corresponds to the /misc partition.
    std::string misc_block_device_;

    // Parses the fstab and find the block device that corresponds to the /misc mount point. On
    // failures, returns an empty string and sets the error message.
    std::string GetMiscBlockDevice(std::string* err);
    // Waits for the block device to be accessible. In recovery mode, recovery can get started and
    // try to access the misc device before the kernel has actually created it.
    bool WaitForDevice(const std::string& blk_device, std::string* err);
    // Reads |size| bytes starting from |offset| from the misc block device. Puts the result in the
    // pre-allocated buffer |dest|. On failures, returns false and sets the error message.
    bool ReadMiscPartition(void* dest, uint32_t size, uint32_t offset, std::string* err);
    // Writes |size| bytes from |source| to the misc block device at |offset|. On failures, returns
    // false and sets the error message.
    bool WriteMiscPartition(const void* source, uint32_t size, uint32_t offset, std::string* err);
    // Updates the command and recovery fields in the input |message|. The recovery field is
    // constructed by the options in |recovery_options|.
    bool UpdateMessageStruct(Message* message, const hidl_vec<hidl_string>& recovery_options);
};

extern "C" BootloaderMessage* HIDL_FETCH_IBootloaderMessage(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace bootloader_message
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_BOOTLOADERMESSAGE_V1_0_BOOTLOADERMESSAGE_H
