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

using android::hardware::IPCThreadState;
using android::hardware::ProcessState;

using android::hardware::bluetooth::V1_0::IBluetoothHci;

int main() {
  const char instance[] = "bluetooth";

  android::sp<IBluetoothHci> service =
      IBluetoothHci::getService(instance, true /* getStub */);
  if (service.get() == nullptr) {
    ALOGE("IBluetoothHci::getService returned NULL, exiting");
    return -1;
  }

  LOG_FATAL_IF(service->isRemote(), "Implementation is REMOTE!");
  service->registerAsService(instance);

  ProcessState::self()->setThreadPoolMaxThreadCount(0);
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}
