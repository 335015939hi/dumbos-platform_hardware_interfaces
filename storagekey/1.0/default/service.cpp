#define LOG_TAG "android.hardware.storagekey@1.0-service"

#include <android/hardware/storagekey/1.0/IStorageKey.h>
#include <hidl/HidlTransportSupport.h>

#include "StorageKey.h"

using android::OK;
using android::sp;
using android::status_t;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::hardware::storagekey::V1_0::IStorageKey;
using android::hardware::storagekey::V1_0::implementation::StorageKey;

int main() {
    configureRpcThreadpool(1, true);

    sp<IStorageKey> storagekey = new StorageKey;
    status_t status = storagekey->registerAsService();
    LOG_ALWAYS_FATAL_IF(status != OK, "Could not register IStorageKey");

    joinRpcThreadpool();
    return 0;
}
