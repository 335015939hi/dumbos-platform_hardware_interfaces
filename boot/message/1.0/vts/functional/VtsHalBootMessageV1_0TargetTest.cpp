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

#define LOG_TAG "boot_message_hidl_hal_test"

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hardware/boot/message/1.0/IBootloaderMessage.h>
#include <android/hardware/boot/message/1.0/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::boot::message::V1_0::CommandResult;
using ::android::hardware::boot::message::V1_0::IBootloaderMessage;
using ::android::hardware::boot::message::V1_0::Message;
using ::android::hardware::boot::message::V1_0::MinimalMessageFieldSize;
using ::android::hardware::boot::message::V1_0::MinimalWipePackageSize;

class BootMessageHidlTest : public testing::TestWithParam<std::string> {
  public:
    void SetUp() override {
        boot_ = IBootloaderMessage::getService(GetParam());
        ASSERT_TRUE(boot_ != nullptr) << "Failed to get BootloaderMessage service.";
    }

    // Clean up the /misc after the test.
    void TearDown() override {
        CommandResult write_status;
        boot_->writeBootloaderMessage(
                {}, [&write_status](const CommandResult& status) { write_status = status; });
        ASSERT_TRUE(write_status.result) << "Failed to clear BCB: " << write_status.errorMessage;
    }

  protected:
    android::sp<IBootloaderMessage> boot_;

    void readBootloaderMessage(Message* read_message);
};

void BootMessageHidlTest::readBootloaderMessage(Message* message) {
    CommandResult read_status;
    boot_->readBootloaderMessage(
            [&message, &read_status](const CommandResult& status, const Message& read_message) {
                read_status = status;
                if (read_status.result) {
                    *message = read_message;
                }
            });
    ASSERT_TRUE(read_status.result) << "Failed to write BCB: " << read_status.errorMessage;
}

TEST_P(BootMessageHidlTest, read_and_write_bootloader_message) {
    // Write the BCB.
    Message message = {
            "command",
            "message1\nmessage2\n",
            "status1",
    };

    CommandResult write_status;
    boot_->writeBootloaderMessage(
            message, [&write_status](const CommandResult& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    readBootloaderMessage(&message_verify);

    ASSERT_EQ(message, message_verify);
}

TEST_P(BootMessageHidlTest, read_and_write_bootloader_message_minsize) {
    // Write the BCB.
    Message message = {
            std::string(static_cast<uint32_t>(MinimalMessageFieldSize::COMMAND_SIZE) - 1, 'a'),
            std::string(static_cast<uint32_t>(MinimalMessageFieldSize::RECOVERY_SIZE) - 1, 'b'),
            std::string(static_cast<uint32_t>(MinimalMessageFieldSize::STAGE_SIZE) - 1, 'c'),
    };

    CommandResult write_status;
    boot_->writeBootloaderMessage(
            message, [&write_status](const CommandResult& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    readBootloaderMessage(&message_verify);

    ASSERT_EQ(message, message_verify);
}

TEST_P(BootMessageHidlTest, update_bootloader_message_recovery_options_empty) {
    // Read and verify.
    android::sp<IBootloaderMessage> bcb = IBootloaderMessage::getService();
    // Write empty vector.
    std::vector<std::string> options;
    CommandResult write_status;
    bcb->updateBootloaderMessageWithRecoveryOptions(
            hidl_vec<hidl_string>(options.begin(), options.end()), true,
            [&write_status](const CommandResult& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    readBootloaderMessage(&message_verify);

    ASSERT_EQ("boot-recovery", message_verify.command);
    ASSERT_EQ("recovery\n", message_verify.recovery);
    ASSERT_TRUE(message_verify.stage.empty());
}

TEST_P(BootMessageHidlTest, update_bootloader_message_recovery_options_long) {
    // Write super long message.
    std::vector<std::string> options;
    for (int i = 0; i < 69; i++) {
        options.push_back("option: " + std::to_string(i));
    }

    std::string expected = "recovery\n" + android::base::Join(options, "\n") + "\n";
    uint32_t recovery_length = static_cast<uint32_t>(MinimalMessageFieldSize::RECOVERY_SIZE) - 1;
    ASSERT_LE(expected.size(), recovery_length);

    // Read and verify.
    android::sp<IBootloaderMessage> bcb = IBootloaderMessage::getService();
    CommandResult write_status;
    bcb->updateBootloaderMessageWithRecoveryOptions(
            hidl_vec<hidl_string>(options.begin(), options.end()), true,
            [&write_status](const CommandResult& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    readBootloaderMessage(&message_verify);

    ASSERT_EQ("boot-recovery", message_verify.command);
    ASSERT_EQ(expected, message_verify.recovery);
    ASSERT_TRUE(message_verify.stage.empty());
}

TEST_P(BootMessageHidlTest, read_write_wipe_package) {
    auto package_size = static_cast<uint32_t>(MinimalWipePackageSize::PACKAGE_SIZE);
    std::vector<uint8_t> wipe_package(package_size, 'a');
    CommandResult write_status;
    boot_->writeWipePackage(
            wipe_package, [&write_status](const CommandResult& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    std::vector<uint8_t> wipe_package_verify;
    CommandResult read_status;
    boot_->readWipePackage(wipe_package.size(),
                           [&read_status, &wipe_package_verify](const CommandResult& status,
                                                                const hidl_vec<uint8_t>& content) {
                               read_status = status;
                               wipe_package_verify = content;
                           });
    ASSERT_TRUE(read_status.result) << "Failed to read wipe package: " << read_status.errorMessage;
    ASSERT_EQ(wipe_package, wipe_package_verify);
}

INSTANTIATE_TEST_SUITE_P(PerInstance, BootMessageHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IBootloaderMessage::descriptor)),
                         android::hardware::PrintInstanceNameToString);
