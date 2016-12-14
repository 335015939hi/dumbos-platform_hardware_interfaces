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

#include <android-base/logging.h>
#include <gtest/gtest.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/IServiceNotification.h>
#include <hwbinder/ProcessState.h>

#include <wifi_hal/driver_tool.h>
#include <wifi_system/interface_tool.h>
#include <wifi_system/supplicant_manager.h>

#include "supplicant_hidl_test_utils.h"

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::ProcessState;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicant;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaIface;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using ::android::hardware::wifi::supplicant::V1_0::ISupplicantP2pIface;
using ::android::hardware::wifi::supplicant::V1_0::IfaceType;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using ::android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using ::android::hidl::manager::V1_0::IServiceNotification;
using ::android::wifi_hal::DriverTool;
using ::android::wifi_system::InterfaceTool;
using ::android::wifi_system::SupplicantManager;

const char kSupplicantServiceName[] = "wpa_supplicant";

struct ServiceNotification : public IServiceNotification {
 public:
  std::mutex mutex_;
  std::condition_variable condition_;

  Return<void> onRegistration(const hidl_string& fq_name,
                              const hidl_string& name,
                              bool preexisting) override {
    if (preexisting) {
      return Void();
    }
    std::unique_lock<std::mutex> lock(mutex_);
    registered_.push_back(std::string(fq_name.c_str()) + "/" + name.c_str());
    lock.unlock();
    condition_.notify_one();
    return Void();
  }

  const std::vector<std::string>& getRegistrations() const {
    return registered_;
  }

 private:
  std::vector<std::string> registered_{};
};

void stopFramework() {
  ASSERT_EQ(std::system("svc wifi disable"), 0);
  sleep(5);
}

void startFramework() {
  ASSERT_EQ(std::system("svc wifi enable"), 0);
}

void stopSupplicant() {
  DriverTool driver_tool;
  SupplicantManager supplicant_manager;

  ASSERT_TRUE(supplicant_manager.StopSupplicant());
  ASSERT_TRUE(driver_tool.UnloadDriver());
  ASSERT_FALSE(supplicant_manager.IsSupplicantRunning());
}

void initilializeDriverAndFirmware() {
  DriverTool driver_tool;
  InterfaceTool iface_tool;
  EXPECT_TRUE(driver_tool.LoadDriver());
  EXPECT_TRUE(driver_tool.ChangeFirmwareMode(DriverTool::kFirmwareModeSta));
  EXPECT_TRUE(iface_tool.SetWifiUpState(true));
}

void startSupplicantAndWaitForHidlService() {
  initilializeDriverAndFirmware();

  android::sp<ServiceNotification> notification = new ServiceNotification();
  ASSERT_TRUE(ISupplicant::registerForNotifications(kSupplicantServiceName,
                                                    notification));
  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  ProcessState::self()->startThreadPool();

  SupplicantManager supplicant_manager;
  ASSERT_TRUE(supplicant_manager.StartSupplicant());
  ASSERT_TRUE(supplicant_manager.IsSupplicantRunning());

  std::unique_lock<std::mutex> lock(notification->mutex_);
  notification->condition_.wait_for(
      lock, std::chrono::milliseconds(100), [&notification]() {
        return notification->getRegistrations().size() >= 1;
      });
  std::vector<std::string> registrations = notification->getRegistrations();
  ASSERT_EQ(registrations.size(), 1u);
}

sp<ISupplicant> getSupplicant() {
  return ISupplicant::getService(kSupplicantServiceName);
}

bool findIfaceOfType(sp<ISupplicant> supplicant,
                     IfaceType desired_type,
                     ISupplicant::IfaceInfo* out_info) {
  bool operation_failed = false;
  std::vector<ISupplicant::IfaceInfo> iface_infos;
  supplicant->listInterfaces([&](const SupplicantStatus& status,
                                 hidl_vec<ISupplicant::IfaceInfo> infos) {
    if (status.code != SupplicantStatusCode::SUCCESS) {
      operation_failed = true;
      return;
    }
    iface_infos = infos;
  });
  if (operation_failed) {
    return false;
  }
  for (const auto& info : iface_infos) {
    if (info.type == desired_type) {
      *out_info = info;
      return true;
    }
  }
  return false;
}

sp<ISupplicantStaIface> getSupplicantStaIface() {
  sp<ISupplicant> supplicant = getSupplicant();
  if (!supplicant.get()) {
    return nullptr;
  }
  ISupplicant::IfaceInfo info;
  if (!findIfaceOfType(supplicant, IfaceType::STA, &info)) {
    return nullptr;
  }
  bool operation_failed = false;
  sp<ISupplicantStaIface> sta_iface;
  supplicant->getInterface(
      info,
      [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
          operation_failed = true;
          return;
        }
        sta_iface = ISupplicantStaIface::castFrom(iface);
      });
  if (operation_failed) {
    return nullptr;
  }
  return sta_iface;
}

sp<ISupplicantStaNetwork> getSupplicantStaNetwork() {
  sp<ISupplicantStaIface> sta_iface = getSupplicantStaIface();
  if (!sta_iface.get()) {
    return nullptr;
  }
  bool operation_failed = false;
  sp<ISupplicantStaNetwork> sta_network;
  sta_iface->addNetwork([&](const SupplicantStatus& status,
                            const sp<ISupplicantNetwork>& network) {
    if (status.code != SupplicantStatusCode::SUCCESS) {
      operation_failed = true;
      return;
    }
    sta_network = ISupplicantStaNetwork::castFrom(network);
  });
  if (operation_failed) {
    return nullptr;
  }
  return sta_network;
}

sp<ISupplicantP2pIface> getSupplicantP2pIface() {
  sp<ISupplicant> supplicant = getSupplicant();
  if (!supplicant.get()) {
    return nullptr;
  }
  ISupplicant::IfaceInfo info;
  if (!findIfaceOfType(supplicant, IfaceType::P2P, &info)) {
    return nullptr;
  }
  bool operation_failed = false;
  sp<ISupplicantP2pIface> p2p_iface;
  supplicant->getInterface(
      info,
      [&](const SupplicantStatus& status, const sp<ISupplicantIface>& iface) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
          operation_failed = true;
          return;
        }
        p2p_iface = ISupplicantP2pIface::castFrom(iface);
      });
  if (operation_failed) {
    return nullptr;
  }
  return p2p_iface;
}

bool turnOnExcessiveLogging() {
  sp<ISupplicant> supplicant = getSupplicant();
  if (!supplicant.get()) {
    return false;
  }
  bool operation_failed = false;
  supplicant->setDebugParams(
      ISupplicant::DebugLevel::EXCESSIVE,
      true,
      true,
      [&](const SupplicantStatus& status) {
        if (status.code != SupplicantStatusCode::SUCCESS) {
          operation_failed = true;
        }
      });
  return !operation_failed;
}
