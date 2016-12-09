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
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/StrongPointer.h>

#include "wifi.h"

using android::hardware::hidl_version;
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::Looper;
using android::hardware::wifi::V1_0::implementation::Wifi;
using android::hardware::wifi::V1_0::IWifi;

int main(int /*argc*/, char** argv) {
  android::base::InitLogging(argv,
                             android::base::LogdLogger(android::base::SYSTEM));
  LOG(INFO) << "Wifi Hal is starting up...";

  // Setup hwbinder service
  android::sp<IWifi> service = new Wifi();
  CHECK_EQ(service->registerAsService("wifi"), android::NO_ERROR)
      << "Failed to register wifi HAL";

  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();

  LOG(INFO) << "Wifi Hal is terminating...";
  return 0;
}
