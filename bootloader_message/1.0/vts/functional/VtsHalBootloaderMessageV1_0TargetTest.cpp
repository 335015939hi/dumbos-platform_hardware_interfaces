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

#define LOG_TAG "bootloader_message_hidl_hal_test"

#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android/hardware/bootloader_message/1.0/IBootloaderMessage.h>
#include <android/hardware/bootloader_message/1.0/types.h>
#include <gtest/gtest.h>
#include <hidl/GtestPrinter.h>
#include <hidl/ServiceManagement.h>

using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::bootloader_message::V1_0::IBootloaderMessage;
using ::android::hardware::bootloader_message::V1_0::Message;
using ::android::hardware::bootloader_message::V1_0::MessageStatus;
using ::android::hardware::bootloader_message::V1_0::MinimalMessageFieldSize;

class BootloaderMessageHidlTest : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        boot_ = IBootloaderMessage::getService(GetParam());
        ASSERT_TRUE(boot_ != nullptr) << "Failed to get BootloaderMessage service.";
    }

    virtual void TearDown() override {
        MessageStatus write_status;
        boot_->WriteBootloaderMessage(
                {}, [&write_status](const MessageStatus& status) { write_status = status; });
        ASSERT_TRUE(write_status.result) << "Failed to clear BCB: " << write_status.errorMessage;
    }

  protected:
    android::sp<IBootloaderMessage> boot_;

    void ReadBootloaderMessage(Message* read_message);
};

void BootloaderMessageHidlTest::ReadBootloaderMessage(Message* message) {
    MessageStatus read_status;
    boot_->ReadBootloaderMessage(
            [&message, &read_status](const MessageStatus& status, const Message& read_message) {
                read_status = status;
                if (read_status.result) {
                    *message = read_message;
                }
            });
    ASSERT_TRUE(read_status.result) << "Failed to write BCB: " << read_status.errorMessage;
}

TEST_P(BootloaderMessageHidlTest, read_and_write_bootloader_message) {
    // Write the BCB.
    Message message = {
            "command",
            "message1\nmessage2\n",
            "status1",
    };

    MessageStatus write_status;
    boot_->WriteBootloaderMessage(
            message, [&write_status](const MessageStatus& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    ReadBootloaderMessage(&message_verify);

    ASSERT_EQ(message, message_verify);
}

TEST_P(BootloaderMessageHidlTest, update_bootloader_message_recovery_options_empty) {
    // Read and verify.
    android::sp<IBootloaderMessage> bcb = IBootloaderMessage::getService();
    // Write empty vector.
    std::vector<std::string> options;
    MessageStatus write_status;
    bcb->UpdateBootloaderMessageWithRecoveryOptions(
            hidl_vec<hidl_string>(options.begin(), options.end()), true,
            [&write_status](const MessageStatus& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    ReadBootloaderMessage(&message_verify);

    ASSERT_EQ("boot-recovery", message_verify.command);
    ASSERT_EQ("recovery\n", message_verify.recovery);
    ASSERT_TRUE(message_verify.stage.empty());
}

TEST_P(BootloaderMessageHidlTest, update_bootloader_message_recovery_options_long) {
    // Write super long message.
    std::vector<std::string> options;
    for (int i = 0; i < 100; i++) {
        options.push_back("option: " + std::to_string(i));
    }

    // Read and verify.
    android::sp<IBootloaderMessage> bcb = IBootloaderMessage::getService();
    MessageStatus write_status;
    bcb->UpdateBootloaderMessageWithRecoveryOptions(
            hidl_vec<hidl_string>(options.begin(), options.end()), true,
            [&write_status](const MessageStatus& status) { write_status = status; });
    ASSERT_TRUE(write_status.result) << "Failed to write BCB: " << write_status.errorMessage;

    // Read and verify.
    Message message_verify;
    ReadBootloaderMessage(&message_verify);

    ASSERT_EQ("boot-recovery", message_verify.command);
    std::string expected = "recovery\n" + android::base::Join(options, "\n") + "\n";
    uint32_t recovery_length = static_cast<uint32_t>(MinimalMessageFieldSize::RECOVERY_SIZE) - 1;
    ASSERT_EQ(expected.substr(0, recovery_length),
              std::string(message_verify.recovery).substr(recovery_length));
    ASSERT_TRUE(message_verify.stage.empty());
}

INSTANTIATE_TEST_SUITE_P(PerInstance, BootloaderMessageHidlTest,
                         testing::ValuesIn(android::hardware::getAllHalInstanceNames(
                                 IBootloaderMessage::descriptor)),
                         android::hardware::PrintInstanceNameToString);
