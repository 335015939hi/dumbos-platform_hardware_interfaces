#define LOG_TAG "android.hardware.bluetooth@1.0-service"
#include <utils/Log.h>

#include <unistd.h>
#include <iostream>

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>

#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

using android::sp;

// libhwbinder:
using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

// Generated HIDL files
using android::hardware::bluetooth::V1_0::IBluetoothHci;

int main() {
  const char instance[] = "bluetooth";

  ALOGI("Service '%s' is starting....", instance);

  android::sp<IBluetoothHci> service =
      IBluetoothHci::getService(instance, true);
  if (service.get() == nullptr) {
    ALOGE("IBluetoothHci::getService returned NULL, exiting");
    return -1;
  }

  ALOGI("Default implementation for '%s' is %s", instance,
        (service->isRemote() ? "REMOTE" : "LOCAL"));
  LOG_FATAL_IF(service->isRemote(), "Implementation is REMOTE!");

  ALOGI("Registering instance %s.", instance);
  service->registerAsService(instance);
  ALOGI("Ready.");

  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}
