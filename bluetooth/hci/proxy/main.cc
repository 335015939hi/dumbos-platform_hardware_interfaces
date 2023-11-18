/*
 * Copyright 2023 The Android Open Source Project
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

#define LOG_TAG "hci_proxy"

#include <aidl/android/hardware/bluetooth/BnBluetoothHci.h>
#include <aidl/android/hardware/bluetooth/BnBluetoothHciCallbacks.h>
#include <aidl/android/hardware/bluetooth/IBluetoothHci.h>
#include <android/binder_ibinder.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/hardware/bluetooth/1.0/types.h>
#include <android/hardware/bluetooth/1.1/IBluetoothHci.h>
#include <android/hardware/bluetooth/1.1/IBluetoothHciCallbacks.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utils/Log.h>

#include <cstdint>
#include <future>
#include <memory>
#include <vector>

using ::android::sp;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using HidlStatus = ::android::hardware::bluetooth::V1_0::Status;
using AidlStatus = ::aidl::android::hardware::bluetooth::Status;
using IBluetoothHci_1_0 = ::android::hardware::bluetooth::V1_0::IBluetoothHci;
using IBluetoothHci_1_1 = ::android::hardware::bluetooth::V1_1::IBluetoothHci;
using IBluetoothHciAidl = ::aidl::android::hardware::bluetooth::IBluetoothHci;
using IBluetoothHciCallbacks_1_0 =
    ::android::hardware::bluetooth::V1_0::IBluetoothHciCallbacks;
using IBluetoothHciCallbacks_1_1 =
    ::android::hardware::bluetooth::V1_1::IBluetoothHciCallbacks;
using ::aidl::android::hardware::bluetooth::BnBluetoothHciCallbacks;

static constexpr int kPort = 9100;
static constexpr char kBluetoothAidlHalServiceName[] =
    "android.hardware.bluetooth.IBluetoothHci/default";

static constexpr uint8_t kHciPacketTypeCommand = 1;
static constexpr uint8_t kHciPacketTypeAclData = 2;
static constexpr uint8_t kHciPacketTypeScoData = 3;
static constexpr uint8_t kHciPacketTypeEvent = 4;
static constexpr uint8_t kHciPacketTypeIsoData = 5;

std::promise<void> init_promise;

class BluetoothHciCallbacksAidl : public BnBluetoothHciCallbacks {
 public:
  BluetoothHciCallbacksAidl() {}

  void setClientFd(int client_fd) { client_fd_ = client_fd; }

  ::ndk::ScopedAStatus initializationComplete(AidlStatus status) override {
    ALOGI("%s status=%d", __func__, status);
    assert(status == AidlStatus::SUCCESS);
    init_promise.set_value();
    return ::ndk::ScopedAStatus::ok();
  }

  ::ndk::ScopedAStatus hciEventReceived(
      const std::vector<uint8_t>& event) override {
    if (client_fd_ == -1) return ::ndk::ScopedAStatus::ok();
    uint8_t packet_type = kHciPacketTypeEvent;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, event.data(), event.size());
    return ::ndk::ScopedAStatus::ok();
  }

  ::ndk::ScopedAStatus aclDataReceived(
      const std::vector<uint8_t>& data) override {
    if (client_fd_ == -1) return ::ndk::ScopedAStatus::ok();
    uint8_t packet_type = kHciPacketTypeAclData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return ::ndk::ScopedAStatus::ok();
  }

  ::ndk::ScopedAStatus scoDataReceived(
      const std::vector<uint8_t>& data) override {
    if (client_fd_ == -1) return ::ndk::ScopedAStatus::ok();
    uint8_t packet_type = kHciPacketTypeScoData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return ::ndk::ScopedAStatus::ok();
  }

  ::ndk::ScopedAStatus isoDataReceived(
      const std::vector<uint8_t>& data) override {
    if (client_fd_ == -1) return ::ndk::ScopedAStatus::ok();
    uint8_t packet_type = kHciPacketTypeIsoData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return ::ndk::ScopedAStatus::ok();
  }

 private:
  int client_fd_ = -1;
};

class BluetoothHciCallbacks_1_0 : public IBluetoothHciCallbacks_1_0 {
 public:
  BluetoothHciCallbacks_1_0() { ALOGV("BluetoothHciCallbacks constructor"); }

  void setClientFd(int client_fd) { client_fd_ = client_fd; }

  Return<void> initializationComplete(HidlStatus status) override {
    // check status
    ALOGI("initializationComplete: status=%d\n", status);
    assert(status == HidlStatus::SUCCESS);
    init_promise.set_value();
    return Void();
  }

  Return<void> hciEventReceived(const hidl_vec<uint8_t>& event) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeEvent;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, event.data(), event.size());
    return Void();
  }

  Return<void> aclDataReceived(const hidl_vec<uint8_t>& data) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeAclData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return Void();
  }

  Return<void> scoDataReceived(const hidl_vec<uint8_t>& data) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeScoData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return Void();
  }

 private:
  int client_fd_ = -1;
};

class BluetoothHciCallbacks_1_1 : public IBluetoothHciCallbacks_1_1 {
 public:
  BluetoothHciCallbacks_1_1() { ALOGV("BluetoothHciCallbacks constructor"); }

  void setClientFd(int client_fd) { client_fd_ = client_fd; }

  Return<void> initializationComplete(HidlStatus status) override {
    // check status
    ALOGI("initializationComplete: status=%d\n", status);
    assert(status == HidlStatus::SUCCESS);
    init_promise.set_value();
    return Void();
  }

  Return<void> hciEventReceived(const hidl_vec<uint8_t>& event) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeEvent;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, event.data(), event.size());
    return Void();
  }

  Return<void> aclDataReceived(const hidl_vec<uint8_t>& data) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeAclData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return Void();
  }

  Return<void> scoDataReceived(const hidl_vec<uint8_t>& data) override {
    if (client_fd_ == -1) return Void();
    uint8_t packet_type = kHciPacketTypeScoData;
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return Void();
  }

  Return<void> isoDataReceived(const hidl_vec<uint8_t>& data) override {
    uint8_t packet_type = kHciPacketTypeIsoData;
    if (client_fd_ == -1) return Void();
    write(client_fd_, &packet_type, sizeof(packet_type));
    write(client_fd_, data.data(), data.size());
    return Void();
  }

 private:
  int client_fd_ = -1;
};

class HalWrapper {
 public:
  virtual ~HalWrapper() = default;
  virtual void setClientFd(int) = 0;
  virtual void sendAclData(std::vector<uint8_t>&) = 0;
  virtual void sendScoData(std::vector<uint8_t>&) = 0;
  virtual void sendHciCommand(std::vector<uint8_t>&) = 0;
  virtual void sendIsoData(std::vector<uint8_t>&) = 0;
  virtual void close() = 0;
};

class HalAidl : public HalWrapper {
 public:
  HalAidl(std::shared_ptr<IBluetoothHciAidl> hal) : hal_(std::move(hal)) {
    cb_ = ::ndk::SharedRefBase::make<BluetoothHciCallbacksAidl>();
    hal_->initialize(cb_);
  }

  static HalAidl* create() {
    ::ndk::SpAIBinder binder(
        AServiceManager_checkService(kBluetoothAidlHalServiceName));
    auto hal = IBluetoothHciAidl::fromBinder(binder);
    if (hal == nullptr) {
      return nullptr;
    }
    ALOGI("Use AIDL");
    // Start a binder thread pool for callbacks.
    ABinderProcess_startThreadPool();
    return new HalAidl(hal);
  }
  virtual ~HalAidl() = default;

  void setClientFd(int client_fd) override {
    ALOGI("%s", __func__);
    cb_->setClientFd(client_fd);
  }
  void sendAclData(std::vector<uint8_t>& data) override {
    hal_->sendAclData(data);
  }
  void sendScoData(std::vector<uint8_t>& data) override {
    hal_->sendScoData(data);
  }
  void sendHciCommand(std::vector<uint8_t>& data) override {
    hal_->sendHciCommand(data);
  }
  void sendIsoData(std::vector<uint8_t>& data) override {
    hal_->sendIsoData(data);
  }
  void close() override { hal_->close(); }

 private:
  std::shared_ptr<IBluetoothHciAidl> hal_;
  std::shared_ptr<BluetoothHciCallbacksAidl> cb_;
};

class Hal_1_1 : public HalWrapper {
 public:
  Hal_1_1(sp<IBluetoothHci_1_1> hal) : hal_(std::move(hal)) {
    cb_ = std::make_unique<BluetoothHciCallbacks_1_1>();
    hal_->initialize(cb_.get());
  }
  static Hal_1_1* create() {
    auto hal = IBluetoothHci_1_1::getService();
    if (hal == nullptr) {
      return nullptr;
    }
    ALOGI("Use HIDL 1.1");
    return new Hal_1_1(hal);
  }
  virtual ~Hal_1_1() = default;

  void setClientFd(int client_fd) override { cb_->setClientFd(client_fd); }
  void sendAclData(std::vector<uint8_t>& data) override {
    hal_->sendAclData(data);
  }
  void sendScoData(std::vector<uint8_t>& data) override {
    hal_->sendScoData(data);
  }
  void sendHciCommand(std::vector<uint8_t>& data) override {
    hal_->sendHciCommand(data);
  }
  void sendIsoData(std::vector<uint8_t>& data) override {
    hal_->sendIsoData(data);
  }
  void close() override { hal_->close(); }

 private:
  sp<IBluetoothHci_1_1> hal_;
  std::unique_ptr<BluetoothHciCallbacks_1_1> cb_;
};

class Hal_1_0 : public HalWrapper {
 public:
  Hal_1_0(sp<IBluetoothHci_1_0> hal) : hal_(std::move(hal)) {
    cb_ = std::make_unique<BluetoothHciCallbacks_1_0>();
    hal_->initialize(cb_.get());
  }
  virtual ~Hal_1_0() = default;

  static Hal_1_0* create() {
    auto hal = IBluetoothHci_1_0::getService();
    if (hal == nullptr) {
      return nullptr;
    }
    ALOGI("Use HIDL 1.0");
    return new Hal_1_0(hal);
  }

  void setClientFd(int client_fd) override {
    ALOGI("%s", __func__);
    cb_->setClientFd(client_fd);
  }
  void sendAclData(std::vector<uint8_t>& data) override {
    hal_->sendAclData(data);
  }
  void sendScoData(std::vector<uint8_t>& data) override {
    hal_->sendScoData(data);
  }
  void sendHciCommand(std::vector<uint8_t>& data) override {
    hal_->sendHciCommand(data);
  }
  void sendIsoData(std::vector<uint8_t>&) override {}
  void close() override { hal_->close(); }

 private:
  sp<IBluetoothHci_1_0> hal_;
  std::unique_ptr<BluetoothHciCallbacks_1_0> cb_;
};

int listen(int port) {
  int retVal;
  struct sockaddr_in servAddr;

  int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (listen_fd < 0) {
    ALOGE("%s: socket() failed, mSockFd=%d, errno=%d", __FUNCTION__, listen_fd,
          errno);
    return -1;
  }
  int reuse = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse,
                 sizeof(reuse)) < 0) {
    ALOGE("setsockopt(SO_REUSEADDR) failed");
    return -1;
  }

  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse,
                 sizeof(reuse)) < 0) {
    ALOGE("setsockopt(SO_REUSEPORT) failed");
    return -1;
  }

  servAddr = {};
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = INADDR_ANY;
  servAddr.sin_port = htons(port);

  retVal = bind(listen_fd, reinterpret_cast<struct sockaddr*>(&servAddr),
                sizeof(servAddr));
  if (retVal < 0) {
    ALOGE("%s: Error on binding: retVal=%d, errno=%d", __FUNCTION__, retVal,
          errno);
    close(listen_fd);
    return -1;
  }

  // Get bound port, showing it on Logcat so daemon can get the port number.
  // (When it's dynamically allocated).
  int len = sizeof(servAddr);
  servAddr = {};
  int getsockname_result =
      getsockname(listen_fd, reinterpret_cast<struct sockaddr*>(&servAddr),
                  reinterpret_cast<socklen_t*>(&len));
  if (getsockname_result < 0) {
    ALOGE("%s: Error on getsockname: retVal=%d, errno=%d", __FUNCTION__,
          getsockname_result, errno);
    close(listen_fd);
    return -1;
  }

  ALOGI("%s: Listening for connections on port %d", __FUNCTION__,
        ntohs(servAddr.sin_port));
  if (::listen(listen_fd, 1) == -1) {
    ALOGE("%s: Error on listening: errno: %d: %s", __FUNCTION__, errno,
          strerror(errno));
    return -1;
  }
  return listen_fd;
}

int accept(int listen_fd) {
  sockaddr_in cliAddr;
  socklen_t cliLen = sizeof(cliAddr);
  int client_fd =
      accept4(listen_fd, reinterpret_cast<struct sockaddr*>(&cliAddr), &cliLen,
              SOCK_CLOEXEC);

  if (client_fd > 0) {
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cliAddr.sin_addr, addr, INET_ADDRSTRLEN);

    ALOGD("%s: Incoming connection received from %s:%d", __FUNCTION__, addr,
          cliAddr.sin_port);
    return client_fd;
  }

  return -1;
}

int main(int argc, char* argv[]) {
  int port = kPort;
  if (argc >= 2) {
    port = atoi(argv[1]);
  }
  int listen_fd = listen(port);
  while (true) {
    int client_fd = accept(listen_fd);
    init_promise = {};
    HalWrapper* hal = HalAidl::create();
    if (hal == nullptr) {
      hal = Hal_1_1::create();
    }
    if (hal == nullptr) {
      hal = Hal_1_0::create();
    }
    if (hal == nullptr) {
      return 0;
    }
    init_promise.get_future().wait();
    hal->setClientFd(client_fd);

    while (true) {
      uint8_t packet_type;
      int len = read(client_fd, &packet_type, sizeof(packet_type));
      if (len <= 0) {
        ALOGI("Socket closed");
        close(client_fd);
        hal->close();
        delete hal;
        break;
      }
      uint8_t buf[1024];
      len = read(client_fd, buf, sizeof(buf));
      std::vector<uint8_t> data(buf, buf + len);

      switch (packet_type) {
        case kHciPacketTypeAclData:
          hal->sendAclData(data);
          break;
        case kHciPacketTypeCommand:
          hal->sendHciCommand(data);
          break;
        case kHciPacketTypeScoData:
          hal->sendScoData(data);
          break;
        case kHciPacketTypeIsoData:
          hal->sendIsoData(data);
          break;
        case kHciPacketTypeEvent:
          ALOGE("Unexpected event packet!");
          break;
      }
    }
  }
}
