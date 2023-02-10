/*
 * Copyright (C) 2022 The Android Open Source Project
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

#define LOG_TAG "RemoteAccessImpl"

#include "RemoteAccessService.h"

#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <stdlib.h>
#include <utils/Log.h>

constexpr char SERVICE_NAME[] = "android.hardware.automotive.remoteaccess.IRemoteAccess/default";

int main(int /* argc */, char* /* argv */[]) {
    ALOGI("Registering RemoteAccessService as service...");

#ifndef GRPC_SERVICE_ADDRESS
    ALOGE("GRPC_SERVICE_ADDRESS is not defined, exiting");
    exit(1);
#endif

    const char* grpcServiceAddress = GRPC_SERVICE_ADDRESS;
    const char* grpcServiceIfname = "";

#ifdef GRPC_SERVICE_IFNAME
    grpcServiceIfname = GRPC_SERVICE_IFNAME;
#endif

    ALOGD("address: %s", grpcServiceAddress);
    ALOGD("ifname: %s", grpcServiceIfname);

    auto service = ndk::SharedRefBase::make<
            android::hardware::automotive::remoteaccess::RemoteAccessService>(grpcServiceAddress,
                                                                              grpcServiceIfname);

    binder_exception_t err = AServiceManager_addService(service->asBinder().get(), SERVICE_NAME);
    if (err != EX_NONE) {
        ALOGE("failed to register android.hardware.automotive.remote.IRemoteAccess service, "
              "exception: %d",
              err);
        exit(1);
    }

    if (!ABinderProcess_setThreadPoolMaxThreadCount(1)) {
        ALOGE("%s", "failed to set thread pool max thread count");
        exit(1);
    }
    ABinderProcess_startThreadPool();

    ALOGI("RemoteAccess service Ready");

    ABinderProcess_joinThreadPool();

    ALOGW("Should not reach here");

    return 0;
}
