/*
 * Copyright 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Gnss.h"

#include <android-base/logging.h>
#include <android/binder_libbinder.h>

#include <binder/RpcServer.h>

namespace android::hardware::gnss {

using aidl::android::hardware::gnss::Gnss;

extern "C" int main() {
    // [TODO] base::SetDefaultTag("gnssproxy");
    // [TODO] base::SetMinimumLogSeverity(base::VERBOSE);
    LOG(DEBUG) << "GNSS RPC service starting...";

    // [not needed for RPC?] ABinderProcess_setThreadPoolMaxThreadCount(1);
    // [not needed for RPC?] ABinderProcess_startThreadPool();

    auto ndkService = ndk::SharedRefBase::make<Gnss>();
    auto platformService = AIBinder_toPlatformBinder(ndkService->asBinder().get());

    // Publish service remotely
    const auto rpcServer = RpcServer::make();
    // [not needed?] const auto serviceDelegator =
    // sp<hardware::gnss::delegator::Gnss>::make(localService);
    rpcServer->setRootObject(platformService);
    // [not needed for RPC?] ProcessState::self()->startThreadPool();
    const auto res = rpcServer->setupInetServer("0.0.0.0", 10203);
    CHECK_EQ(res, OK) << "Couldn't setup Inet server for GNSS";

    // Ready to run!
    LOG(INFO) << "GNSS RPC service ready to serve";
    rpcServer->join();  // should not return
    LOG(ERROR) << "GNSS RPC service server quitting";
    return EXIT_FAILURE;
}

}  // namespace android::hardware::gnss
