/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "bluetooth_hidl_hal_test"
#include <android-base/logging.h>

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>
#include <android/hardware/bluetooth/1.0/IBluetoothHciCallbacks.h>
#include <android/hardware/bluetooth/1.0/types.h>
#include <hardware/bluetooth.h>

#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>
#include <queue>

using ::android::hardware::bluetooth::V1_0::IBluetoothHci;
using ::android::hardware::bluetooth::V1_0::IBluetoothHciCallbacks;
using ::android::hardware::bluetooth::V1_0::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define Bluetooth_HCI_SERVICE_NAME "bluetooth_hci"

#define NUM_HCI_COMMANDS_BANDWIDTH 1000
#define NUM_SCO_PACKETS_BANDWIDTH 1000
#define SCO_TEST_PACKET_SIZE 10000  // Must be greater than 4
#define NUM_ACL_PACKETS_BANDWIDTH 1000
#define ACL_TEST_PACKET_SIZE 10000  // Must be greater than 4
#define WAIT_FOR_HCI_EVENT_TIMEOUT std::chrono::milliseconds(1000)
#define WAIT_FOR_SCO_DATA_TIMEOUT std::chrono::milliseconds(1000)
#define WAIT_FOR_ACL_DATA_TIMEOUT std::chrono::milliseconds(1000)

#define COMMAND_HCI_WRITE_LOOPBACK_MODE_LOCAL \
  { 0x02, 0x18, 0x01, 0x01 }
#define COMMAND_HCI_RESET \
  { 0x03, 0x0c, 0x00 }
#define COMMAND_HCI_WRITE_LOCAL_NAME \
  { 0x13, 0x0c, 0xf8 }
#define HCI_STATUS_SUCCESS 0x00

// {0x0e, 0x04, 0x01, 0x03, 0x0c, 0x00}
// {0x0f, 0x04, 0x00, 0x01, 0x03, 0x0c}
#define EVENT_CONNECTION_COMPLETE 0x03
#define EVENT_COMMAND_COMPLETE 0x0e
#define EVENT_COMMAND_STATUS 0x0f
#define EVENT_LOOPBACK_COMMAND 0x19

#define EVENT_CODE_BYTE 0
#define EVENT_LENGTH_BYTE 1
#define EVENT_FIRST_PAYLOAD_BYTE 2
#define EVENT_COMMAND_STATUS_STATUS_BYTE 2
#define EVENT_COMMAND_STATUS_ALLOWED_PACKETS_BYTE 3
#define EVENT_COMMAND_STATUS_OPCODE_LSBYTE 4  // Bytes 4 and 5
#define EVENT_COMMAND_COMPLETE_ALLOWED_PACKETS_BYTE 2
#define EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE 3  // Bytes 3 and 4
#define EVENT_COMMAND_COMPLETE_STATUS_BYTE 5

#define EVENT_CONNECTION_COMPLETE_PARAM_LENGTH 11
#define EVENT_CONNECTION_COMPLETE_TYPE 11
#define EVENT_CONNECTION_COMPLETE_TYPE_SCO 0
#define EVENT_CONNECTION_COMPLETE_TYPE_ACL 1
#define EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE 3
#define EVENT_COMMAND_STATUS_LENGTH 4
#define EVENT_COMMAND_COMPLETE_LENGTH 4

#define ACL_BROADCAST_ACTIVE_SLAVE (0x1 << 4)
#define ACL_PACKET_BOUNDARY_COMPLETE (0x3 << 6)

// The main test class for Bluetooth HIDL HAL.
class BluetoothHidlTest : public ::testing::Test {
 public:
  virtual void SetUp() override {
    // currently test passthrough mode only
    bluetooth = IBluetoothHci::getService(Bluetooth_HCI_SERVICE_NAME, true);
    ASSERT_NE(bluetooth, nullptr);

    bluetooth_cb = new BluetoothHciCallbacks(*this);
    ASSERT_NE(bluetooth_cb, nullptr);

    event_count = 0;
    acl_count = 0;
    sco_count = 0;
    event_cb_count = 0;
    acl_cb_count = 0;
    sco_cb_count = 0;
  }

  virtual void TearDown() override {
    EXPECT_EQ(static_cast<size_t>(0), event_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), sco_queue.size());
    EXPECT_EQ(static_cast<size_t>(0), acl_queue.size());
  }

  // Functions called from within tests in loopback mode
  void sendAndCheckHCI(int num_packets);
  void sendAndCheckSCO(int num_packets, uint16_t handle);
  void sendAndCheckACL(int num_packets, uint16_t handle);

  /* Inform the test about an event callback */
  inline void notify_event_received() {
    std::unique_lock<std::mutex> lock(event_mutex);
    event_count++;
    event_condition.notify_one();
  }

  /* Test code calls this function to wait for an event callback */
  inline void wait_for_event() {
    std::unique_lock<std::mutex> lock(event_mutex);

    auto start_time = std::chrono::steady_clock::now();
    while (event_count == 0)
      if (event_condition.wait_until(lock,
                                     start_time + WAIT_FOR_HCI_EVENT_TIMEOUT) ==
          std::cv_status::timeout)
        return;
    event_count--;
  }

  /* Inform the test about an acl data callback */
  inline void notify_acl_data_received() {
    std::unique_lock<std::mutex> lock(acl_mutex);
    acl_count++;
    acl_condition.notify_one();
  }

  /* Test code calls this function to wait for an acl data callback */
  inline void wait_for_acl() {
    std::unique_lock<std::mutex> lock(acl_mutex);

    while (acl_count == 0)
      acl_condition.wait_until(
          lock, std::chrono::steady_clock::now() + WAIT_FOR_ACL_DATA_TIMEOUT);
    acl_count--;
  }

  /* Inform the test about a sco data callback */
  inline void notify_sco_data_received() {
    std::unique_lock<std::mutex> lock(sco_mutex);
    sco_count++;
    sco_condition.notify_one();
  }

  /* Test code calls this function to wait for a sco data callback */
  inline void wait_for_sco() {
    std::unique_lock<std::mutex> lock(sco_mutex);

    while (sco_count == 0)
      sco_condition.wait_until(
          lock, std::chrono::steady_clock::now() + WAIT_FOR_SCO_DATA_TIMEOUT);
    sco_count--;
  }

  // A simple test implementation of BluetoothHciCallbacks.
  class BluetoothHciCallbacks : public IBluetoothHciCallbacks {
    BluetoothHidlTest& parent_;

   public:
    BluetoothHciCallbacks(BluetoothHidlTest& parent) : parent_(parent){};

    virtual ~BluetoothHciCallbacks() = default;

    Return<void> hciEventReceived(
        const ::android::hardware::hidl_vec<uint8_t>& event) override {
      parent_.event_cb_count++;
      parent_.event_queue.push(event);
      parent_.notify_event_received();
      ALOGV("Event received (length = %d)", static_cast<int>(event.size()));
      return Void();
    };

    Return<void> aclDataReceived(
        const ::android::hardware::hidl_vec<uint8_t>& data) override {
      parent_.acl_cb_count++;
      parent_.acl_queue.push(data);
      parent_.notify_acl_data_received();
      return Void();
    };

    Return<void> scoDataReceived(
        const ::android::hardware::hidl_vec<uint8_t>& data) override {
      parent_.sco_cb_count++;
      parent_.sco_queue.push(data);
      parent_.notify_sco_data_received();
      return Void();
    };
  };

  sp<IBluetoothHci> bluetooth;
  sp<IBluetoothHciCallbacks> bluetooth_cb;
  std::queue<hidl_vec<uint8_t>> event_queue;
  std::queue<hidl_vec<uint8_t>> acl_queue;
  std::queue<hidl_vec<uint8_t>> sco_queue;

  int event_cb_count;
  int sco_cb_count;
  int acl_cb_count;

 private:
  std::mutex event_mutex;
  std::mutex sco_mutex;
  std::mutex acl_mutex;
  std::condition_variable event_condition;
  std::condition_variable sco_condition;
  std::condition_variable acl_condition;
  int event_count;
  int sco_count;
  int acl_count;
};

// A class for test environment setup (kept since this file is a template).
class BluetoothHidlEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

 private:
};

TEST_F(BluetoothHidlTest, InitializeAndClose) {
  // Collision with android::hardware::Status
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));
  bluetooth->close();
  EXPECT_EQ(0, event_cb_count);
}

TEST_F(BluetoothHidlTest, HciReset) {
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));

  hidl_vec<uint8_t> cmd = COMMAND_HCI_RESET;
  bluetooth->sendHciCommand(cmd);

  // Allow intermediate COMMAND_STATUS events
  int status_event_count = 0;
  hidl_vec<uint8_t> event;
  do {
    wait_for_event();
    event = event_queue.front();
    event_queue.pop();
    if (event[EVENT_CODE_BYTE] == EVENT_COMMAND_STATUS) {
      EXPECT_EQ(EVENT_COMMAND_STATUS_LENGTH, event[EVENT_LENGTH_BYTE]);
      EXPECT_EQ(cmd[0], event[EVENT_COMMAND_STATUS_OPCODE_LSBYTE]);
      EXPECT_EQ(cmd[1], event[EVENT_COMMAND_STATUS_OPCODE_LSBYTE + 1]);
      EXPECT_EQ(event[EVENT_COMMAND_STATUS_STATUS_BYTE], HCI_STATUS_SUCCESS);
      status_event_count++;
    }
  } while (event[EVENT_CODE_BYTE] == EVENT_COMMAND_STATUS);

  EXPECT_EQ(EVENT_COMMAND_COMPLETE, event[EVENT_CODE_BYTE]);
  EXPECT_EQ(cmd[0], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE]);
  EXPECT_EQ(cmd[1], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE + 1]);
  EXPECT_EQ(HCI_STATUS_SUCCESS, event[EVENT_COMMAND_COMPLETE_STATUS_BYTE]);
  bluetooth->close();

  EXPECT_EQ(status_event_count + 1, event_cb_count);
}

TEST_F(BluetoothHidlTest, BadPacketHciReset) {
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));

  std::vector<uint8_t> short_cmd = COMMAND_HCI_RESET;
  // Remove parameter size field
  short_cmd.pop_back();
  hidl_vec<uint8_t> cmd = short_cmd;
  bluetooth->sendHciCommand(cmd);

  // Wait to see if there is a response
  wait_for_event();

  bluetooth->close();

  // Make sure that no events were received
  EXPECT_EQ(0, event_cb_count);
}

TEST_F(BluetoothHidlTest, WriteLoopbackMode) {
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));

  hidl_vec<uint8_t> cmd = COMMAND_HCI_WRITE_LOOPBACK_MODE_LOCAL;
  bluetooth->sendHciCommand(cmd);

  // Receive connection complete events with data channels
  int connection_event_count = 0;
  hidl_vec<uint8_t> event;
  do {
    wait_for_event();
    event = event_queue.front();
    event_queue.pop();
    EXPECT_TRUE(event.size() > 0);
    if (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE) {
      // Save handles in future tests
      uint16_t handle = event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE] |
                        event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE + 1] << 8;
      uint8_t connection_type = event[EVENT_CONNECTION_COMPLETE_TYPE];
      EXPECT_EQ(event[EVENT_LENGTH_BYTE],
                EVENT_CONNECTION_COMPLETE_PARAM_LENGTH);
      EXPECT_TRUE(connection_type == EVENT_CONNECTION_COMPLETE_TYPE_SCO ||
                  connection_type == EVENT_CONNECTION_COMPLETE_TYPE_ACL);
      ALOGD("Connect complete type = %d handle = %d",
            event[EVENT_CONNECTION_COMPLETE_TYPE], handle);
      connection_event_count++;
    }
  } while (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE);

  EXPECT_TRUE(event_cb_count > 2);
  EXPECT_TRUE(connection_event_count > 0);

  EXPECT_EQ(EVENT_COMMAND_COMPLETE, event[EVENT_CODE_BYTE]);
  EXPECT_EQ(cmd[0], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE]);
  EXPECT_EQ(cmd[1], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE + 1]);
  EXPECT_EQ(HCI_STATUS_SUCCESS, event[EVENT_COMMAND_COMPLETE_STATUS_BYTE]);
  bluetooth->close();
  EXPECT_EQ(connection_event_count + 1, event_cb_count);
}

void BluetoothHidlTest::sendAndCheckHCI(int num_packets) {
  for (int n = 0; n < num_packets; n++) {
    // Send an HCI packet
    std::vector<uint8_t> write_name = COMMAND_HCI_WRITE_LOCAL_NAME;
    // With a name
    char new_name[] = "John Jacob Jingleheimer Schmidt ___________________0";
    size_t new_name_length = strlen(new_name);
    for (size_t i = 0; i < new_name_length; i++)
      write_name.push_back(static_cast<uint8_t>(new_name[i]));
    // And the packet number
    {
      size_t i = new_name_length - 1;
      for (int digits = n; digits > 0; digits = digits / 10, i--)
        write_name[i] = static_cast<uint8_t>('0' + digits % 10);
    }
    // And padding
    for (size_t i = 0; i < 248 - new_name_length; i++)
      write_name.push_back(static_cast<uint8_t>(0));

    hidl_vec<uint8_t> cmd = write_name;
    bluetooth->sendHciCommand(cmd);

    // Check the loopback of the HCI packet
    wait_for_event();
    hidl_vec<uint8_t> event = event_queue.front();
    event_queue.pop();

    EXPECT_EQ(EVENT_LOOPBACK_COMMAND, event[EVENT_CODE_BYTE]);
    size_t compare_length =
        (cmd.size() > static_cast<size_t>(0xff) ? static_cast<size_t>(0xff)
                                                : cmd.size());
    EXPECT_EQ(compare_length, event[EVENT_LENGTH_BYTE]);

    for (size_t i = 0; i < compare_length; i++)
      EXPECT_EQ(cmd[i], event[EVENT_FIRST_PAYLOAD_BYTE + i]);
  }
}

void BluetoothHidlTest::sendAndCheckSCO(int num_packets, uint16_t handle) {
  for (int n = 0; n < num_packets; n++) {
    // Send a SCO packet
    hidl_vec<uint8_t> sco_packet;
    uint16_t num_bytes = SCO_TEST_PACKET_SIZE - 4;  // Account for the header.
    std::vector<uint8_t> sco_vector;
    sco_vector.push_back(static_cast<uint8_t>(handle & 0xff));
    sco_vector.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8));
    sco_vector.push_back(static_cast<uint8_t>(num_bytes & 0xff));
    sco_vector.push_back(static_cast<uint8_t>((num_bytes & 0xff00) >> 8));
    for (size_t i = 0; i < num_bytes; i++) {
      sco_vector.push_back(static_cast<uint8_t>(i + n));
    }
    sco_packet = sco_vector;
    bluetooth->sendScoData(sco_vector);

    // Check the loopback of the SCO packet
    wait_for_sco();
    hidl_vec<uint8_t> sco_loopback = sco_queue.front();
    sco_queue.pop();

    for (size_t i = 0; i < sco_packet.size(); i++)
      EXPECT_EQ(sco_packet[i], sco_loopback[i]);
  }
}

void BluetoothHidlTest::sendAndCheckACL(int num_packets, uint16_t handle) {
  for (int n = 0; n < num_packets; n++) {
    // Send an ACL packet
    hidl_vec<uint8_t> acl_packet;
    uint16_t num_bytes = ACL_TEST_PACKET_SIZE - 4;  // Account for the header.
    std::vector<uint8_t> acl_vector;
    acl_vector.push_back(static_cast<uint8_t>(handle & 0xff));
    acl_vector.push_back(static_cast<uint8_t>((handle & 0x0f00) >> 8));
    acl_vector.push_back(static_cast<uint8_t>(num_bytes & 0xff));
    acl_vector.push_back(static_cast<uint8_t>((num_bytes & 0xff00) >> 8) |
                         ACL_BROADCAST_ACTIVE_SLAVE |
                         ACL_PACKET_BOUNDARY_COMPLETE);
    for (size_t i = 0; i < num_bytes; i++) {
      acl_vector.push_back(static_cast<uint8_t>(i + n));
    }
    acl_packet = acl_vector;
    bluetooth->sendAclData(acl_vector);

    // Check the loopback of the ACL packet
    wait_for_acl();
    hidl_vec<uint8_t> acl_loopback = acl_queue.front();
    acl_queue.pop();

    for (size_t i = 0; i < acl_packet.size(); i++)
      EXPECT_EQ(acl_packet[i], acl_loopback[i]);
  }
}

TEST_F(BluetoothHidlTest, LoopbackModeSinglePackets) {
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));

  hidl_vec<uint8_t> cmd = COMMAND_HCI_WRITE_LOOPBACK_MODE_LOCAL;
  bluetooth->sendHciCommand(cmd);

  // Receive connection complete events with data channels
  int connection_event_count = 0;
  hidl_vec<uint8_t> event;
  std::vector<uint16_t> sco_connection_handles;
  std::vector<uint16_t> acl_connection_handles;
  do {
    wait_for_event();
    event = event_queue.front();
    event_queue.pop();
    EXPECT_TRUE(event.size() > 0);
    if (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE) {
      EXPECT_EQ(event[EVENT_LENGTH_BYTE],
                EVENT_CONNECTION_COMPLETE_PARAM_LENGTH);
      uint8_t connection_type = event[EVENT_CONNECTION_COMPLETE_TYPE];

      EXPECT_TRUE(connection_type == EVENT_CONNECTION_COMPLETE_TYPE_SCO ||
                  connection_type == EVENT_CONNECTION_COMPLETE_TYPE_ACL);

      // Save handles
      uint16_t handle = event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE] |
                        event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE + 1] << 8;
      if (connection_type == EVENT_CONNECTION_COMPLETE_TYPE_SCO)
        sco_connection_handles.push_back(handle);
      else
        acl_connection_handles.push_back(handle);

      ALOGD("Connect complete type = %d handle = %d",
            event[EVENT_CONNECTION_COMPLETE_TYPE], handle);
      connection_event_count++;
    }
  } while (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE);

  EXPECT_TRUE(event_cb_count > 2);
  EXPECT_TRUE(connection_event_count > 0);

  EXPECT_EQ(EVENT_COMMAND_COMPLETE, event[EVENT_CODE_BYTE]);
  EXPECT_EQ(cmd[0], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE]);
  EXPECT_EQ(cmd[1], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE + 1]);
  EXPECT_EQ(HCI_STATUS_SUCCESS, event[EVENT_COMMAND_COMPLETE_STATUS_BYTE]);

  sendAndCheckHCI(1);

  if (sco_connection_handles.size() > 0)
    sendAndCheckSCO(1, sco_connection_handles[0]);

  if (acl_connection_handles.size() > 0)
    sendAndCheckACL(1, acl_connection_handles[0]);

  bluetooth->close();
  EXPECT_EQ(connection_event_count + 2, event_cb_count);
  EXPECT_EQ(1, sco_cb_count);
  EXPECT_EQ(1, acl_cb_count);
}

TEST_F(BluetoothHidlTest, LoopbackModeBandwidth) {
  EXPECT_EQ(android::hardware::bluetooth::V1_0::Status::SUCCESS,
            bluetooth->initialize(bluetooth_cb));

  hidl_vec<uint8_t> cmd = COMMAND_HCI_WRITE_LOOPBACK_MODE_LOCAL;
  bluetooth->sendHciCommand(cmd);

  // Receive connection complete events with data channels
  int connection_event_count = 0;
  hidl_vec<uint8_t> event;
  std::vector<uint16_t> sco_connection_handles;
  std::vector<uint16_t> acl_connection_handles;
  do {
    wait_for_event();
    event = event_queue.front();
    event_queue.pop();
    EXPECT_TRUE(event.size() > 0);
    if (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE) {
      EXPECT_EQ(event[EVENT_LENGTH_BYTE],
                EVENT_CONNECTION_COMPLETE_PARAM_LENGTH);
      uint8_t connection_type = event[EVENT_CONNECTION_COMPLETE_TYPE];

      EXPECT_TRUE(connection_type == EVENT_CONNECTION_COMPLETE_TYPE_SCO ||
                  connection_type == EVENT_CONNECTION_COMPLETE_TYPE_ACL);

      // Save handles
      uint16_t handle = event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE] |
                        event[EVENT_CONNECTION_COMPLETE_HANDLE_LSBYTE + 1] << 8;
      if (connection_type == EVENT_CONNECTION_COMPLETE_TYPE_SCO)
        sco_connection_handles.push_back(handle);
      else
        acl_connection_handles.push_back(handle);

      ALOGD("Connect complete type = %d handle = %d",
            event[EVENT_CONNECTION_COMPLETE_TYPE], handle);
      connection_event_count++;
    }
  } while (event[EVENT_CODE_BYTE] == EVENT_CONNECTION_COMPLETE);

  EXPECT_TRUE(event_cb_count > 2);
  EXPECT_TRUE(connection_event_count > 0);
  EXPECT_EQ(EVENT_COMMAND_COMPLETE, event[EVENT_CODE_BYTE]);
  EXPECT_EQ(cmd[0], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE]);
  EXPECT_EQ(cmd[1], event[EVENT_COMMAND_COMPLETE_OPCODE_LSBYTE + 1]);
  EXPECT_EQ(HCI_STATUS_SUCCESS, event[EVENT_COMMAND_COMPLETE_STATUS_BYTE]);

  sendAndCheckHCI(NUM_HCI_COMMANDS_BANDWIDTH);

  if (sco_connection_handles.size() > 0)
    sendAndCheckSCO(NUM_SCO_PACKETS_BANDWIDTH, sco_connection_handles[0]);

  if (acl_connection_handles.size() > 0)
    sendAndCheckACL(NUM_ACL_PACKETS_BANDWIDTH, acl_connection_handles[0]);

  bluetooth->close();
  EXPECT_EQ(NUM_HCI_COMMANDS_BANDWIDTH + connection_event_count + 1,
            event_cb_count);
  EXPECT_EQ(NUM_SCO_PACKETS_BANDWIDTH, sco_cb_count);
  EXPECT_EQ(NUM_ACL_PACKETS_BANDWIDTH, acl_cb_count);
}

int main(int argc, char** argv) {
  ::testing::AddGlobalTestEnvironment(new BluetoothHidlEnvironment);
  ::testing::InitGoogleTest(&argc, argv);
  int status = RUN_ALL_TESTS();
  ALOGI("Test result = %d", status);
  return status;
}
