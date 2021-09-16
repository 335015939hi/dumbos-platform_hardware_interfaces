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

#include <aidl/android/hardware/hypervisor/BnHypervisor.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::hypervisor::BnHypervisor;

class Hypervisor : public BnHypervisor {
    ndk::ScopedAStatus getVersionString(std::string* _aidl_return) override {
        *_aidl_return = "example hypervisor version 3.1415";
        return ndk::ScopedAStatus::ok();
    }

    virtual inline binder_status_t dump(int fd, const char** args, uint32_t numArgs) {
        if (fd < 0) return STATUS_OK;

        (void)args;
        (void)numArgs;

        std::string hyp;
        getVersionString(&hyp);
        dprintf(fd, "Hypervisor HAL: detected hypervisor %s\n", hyp.c_str());

        return STATUS_OK;
    }
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    std::shared_ptr<Hypervisor> hyp = ndk::SharedRefBase::make<Hypervisor>();

    const std::string instance = std::string() + Hypervisor::descriptor + "/default";
    binder_status_t status = AServiceManager_addService(hyp->asBinder().get(), instance.c_str());
    CHECK(status == STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
