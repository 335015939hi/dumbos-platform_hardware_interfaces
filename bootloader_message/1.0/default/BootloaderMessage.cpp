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

#define LOG_TAG "android.hardware.bootloader_message@1.0-impl"

#include <inttypes.h>
#include <sys/stat.h>

#include <tuple>
#include <vector>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/unique_fd.h>
#include <fstab/fstab.h>

#include "BootloaderMessage.h"

namespace android {
namespace hardware {
namespace bootloader_message {
namespace V1_0 {
namespace implementation {

using android::fs_mgr::Fstab;
using android::fs_mgr::ReadDefaultFstab;

// Spaces used by misc partition are as below:
// 0   - 2K     For bootloader_message
// 2K  - 16K    Used by Vendor's bootloader
// 16K - 64K    Used by uncrypt and recovery to store wipe_package for A/B devices
// Note that these offsets are admitted by bootloader,recovery and uncrypt, so they
// are not configurable without changing all of them.
constexpr uint32_t BOOTLOADER_MESSAGE_OFFSET_IN_MISC = 0;
constexpr uint32_t WIPE_PACKAGE_OFFSET_IN_MISC = 16 * 1024;
constexpr uint32_t WIPE_PACKAGE_SIZE_MAX = 48 * 1024;

std::string BootloaderMessage::GetMiscBlockDevice(std::string* err) {
    if (!misc_block_device_.empty()) {
        return misc_block_device_;
    }
    Fstab fstab;
    if (!ReadDefaultFstab(&fstab)) {
        *err = "failed to read default fstab";
        return "";
    }
    for (const auto& entry : fstab) {
        if (entry.mount_point == "/misc") {
            misc_block_device_ = entry.blk_device;
            return entry.blk_device;
        }
    }

    *err = "failed to find /misc partition";
    return "";
}

bool BootloaderMessage::WaitForDevice(const std::string& blk_device, std::string* err) {
    int tries = 0;
    int ret;
    err->clear();
    do {
        ++tries;
        struct stat buf;
        ret = stat(blk_device.c_str(), &buf);
        if (ret == -1) {
            *err += android::base::StringPrintf("failed to stat %s try %d: %s\n",
                                                blk_device.c_str(), tries, strerror(errno));
            sleep(1);
        }
    } while (ret && tries < 10);

    if (ret) {
        *err += android::base::StringPrintf("failed to stat %s\n", blk_device.c_str());
    }
    return ret == 0;
}

bool BootloaderMessage::ReadMiscPartition(void* dest, uint32_t size, uint32_t offset,
                                          std::string* err) {
    auto misc_blk_device = GetMiscBlockDevice(err);
    if (misc_blk_device.empty()) {
        return false;
    }
    if (!WaitForDevice(misc_blk_device, err)) {
        return false;
    }

    android::base::unique_fd fd(open(misc_blk_device.c_str(), O_RDONLY));
    if (fd == -1) {
        *err = android::base::StringPrintf("failed to open %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    if (lseek(fd, static_cast<off_t>(offset), SEEK_SET) != static_cast<off_t>(offset)) {
        *err = android::base::StringPrintf("failed to lseek %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    if (!android::base::ReadFully(fd, dest, size)) {
        *err = android::base::StringPrintf("failed to read %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    return true;
}

bool BootloaderMessage::WriteMiscPartition(const void* source, uint32_t size, uint32_t offset,
                                           std::string* err) {
    auto misc_blk_device = GetMiscBlockDevice(err);
    if (misc_blk_device.empty()) {
        return false;
    }
    if (!WaitForDevice(misc_blk_device, err)) {
        return false;
    }

    android::base::unique_fd fd(open(misc_blk_device.c_str(), O_WRONLY));
    if (fd == -1) {
        *err = android::base::StringPrintf("failed to open %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    if (lseek(fd, static_cast<off_t>(offset), SEEK_SET) != static_cast<off_t>(offset)) {
        *err = android::base::StringPrintf("failed to lseek %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    if (!android::base::WriteFully(fd, source, size)) {
        *err = android::base::StringPrintf("failed to write %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    if (fsync(fd) == -1) {
        *err = android::base::StringPrintf("failed to fsync %s: %s", misc_blk_device.c_str(),
                                           strerror(errno));
        return false;
    }
    return true;
}

bool BootloaderMessage::UpdateMessageStruct(Message* message,
                                            const hidl_vec<hidl_string>& recovery_options) {
    if (!message) return false;
    // Replace the command & recovery fields.
    message->command = "boot-recovery";
    std::string recovery = "recovery\n";
    // Concatenate the recovery options, and put a new line after every option.
    for (const auto& s : recovery_options) {
        recovery += s;
        if (recovery.back() != '\n') {
            recovery += '\n';
        }
    }
    message->recovery = recovery;

    return true;
}

Return<void> BootloaderMessage::ReadBootloaderMessage(ReadBootloaderMessage_cb _hidl_cb) {
    std::string err;
    MessageImpl message_impl{};
    if (!ReadMiscPartition(&message_impl, sizeof(message_impl), BOOTLOADER_MESSAGE_OFFSET_IN_MISC,
                           &err)) {
        _hidl_cb({false, err}, {});
        return Void();
    }

    // Converts the MessageImpl to Message struct.
    Message message = {
            .command = message_impl.command,
            .recovery = message_impl.recovery,
            .stage = message_impl.stage,
    };

    _hidl_cb({true, ""}, message);
    return Void();
}

Return<void> BootloaderMessage::WriteBootloaderMessage(const Message& message,
                                                       WriteBootloaderMessage_cb _hidl_cb) {
    std::vector<std::tuple<std::string, size_t, size_t>> field_size_limits = {
            {"command", message.command.size(), sizeof(MessageImpl::command)},
            {"recovery", message.recovery.size(), sizeof(MessageImpl::recovery)},
            {"stage", message.stage.size(), sizeof(MessageImpl::stage)},
    };
    // Check the size of the each fields, and make sure they fit into the storage. We report an
    // error here instead of writing partial fields.
    for (const auto& [field_name, field_size, size_limit] : field_size_limits) {
        if (field_size > size_limit - 1) {
            auto err = android::base::StringPrintf("Message %s size %zu exceeds limit %zu.",
                                                   field_name.c_str(), field_size, size_limit);
            _hidl_cb({false, err});
            return Void();
        }
    }

    // Construct the MessageImpl from the input.
    MessageImpl message_impl = {};
    memcpy(message_impl.command, message.command.c_str(), message.command.size());
    memcpy(message_impl.recovery, message.recovery.c_str(), message.recovery.size());
    memcpy(message_impl.stage, message.stage.c_str(), message.stage.size());

    std::string err;
    if (!WriteMiscPartition(&message_impl, sizeof(message_impl), BOOTLOADER_MESSAGE_OFFSET_IN_MISC,
                            &err)) {
        _hidl_cb({false, err});
        return Void();
    }
    _hidl_cb({true, ""});
    return Void();
}

Return<void> BootloaderMessage::UpdateBootloaderMessageWithRecoveryOptions(
        const hidl_vec<hidl_string>& options, bool clear_fields,
        UpdateBootloaderMessageWithRecoveryOptions_cb _hidl_cb) {
    Message message;
    // To preserve the contents in other fields, we need to read the message from the storage first.
    if (!clear_fields) {
        ReadBootloaderMessage([&message](const CommandResult& status, const Message& read_message) {
            if (!status.result) {
                LOG(ERROR) << "Failed to read bootloader message " << status.errorMessage;
                return;
            }
            message = read_message;
        });
    }

    UpdateMessageStruct(&message, options);
    return WriteBootloaderMessage(message, _hidl_cb);
}

Return<void> BootloaderMessage::ReadWipePackage(uint32_t package_size,
                                                ReadWipePackage_cb _hidl_cb) {
    if (package_size > WIPE_PACKAGE_SIZE_MAX) {
        auto err = android::base::StringPrintf(
                "wipe package size is too large: "
                "%" PRIu32,
                package_size);
        _hidl_cb({false, err}, {});
        return Void();
    }

    std::vector<uint8_t> package_data(package_size, 0);
    std::string err;
    if (!ReadMiscPartition(package_data.data(), package_size, WIPE_PACKAGE_OFFSET_IN_MISC, &err)) {
        _hidl_cb({false, err}, {});
        return Void();
    }

    _hidl_cb({true, ""}, package_data);
    return Void();
}

Return<void> BootloaderMessage::WriteWipePackage(const hidl_vec<uint8_t>& package_data,
                                                 WriteWipePackage_cb _hidl_cb) {
    if (package_data.size() > WIPE_PACKAGE_SIZE_MAX) {
        auto err = android::base::StringPrintf("wipe package size is too large: %zu",
                                               package_data.size());
        _hidl_cb({false, err});
        return Void();
    }

    std::string err;
    if (!WriteMiscPartition(package_data.data(), package_data.size(), WIPE_PACKAGE_OFFSET_IN_MISC,
                            &err)) {
        _hidl_cb({false, err});
        return Void();
    }

    _hidl_cb({true, ""});
    return Void();
}

BootloaderMessage* HIDL_FETCH_IBootloaderMessage(const char* /* name */) {
    return new BootloaderMessage();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bootloader_message
}  // namespace hardware
}  // namespace android
