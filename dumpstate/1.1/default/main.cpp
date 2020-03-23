/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android-base/properties.h>
#include <android/hardware/dumpstate/1.1/IDumpstateDevice.h>
#include <android/hardware/dumpstate/1.1/types.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

namespace {
using ::android::hardware::hidl_handle;
using ::android::hardware::Return;
using ::android::hardware::Void;

namespace ahd = ::android::hardware::dumpstate;
using IDumpstateDevice1 = ahd::V1_1::IDumpstateDevice;
using DumpstateMode1 = ahd::V1_1::DumpstateMode;
using DumpstateStatus1 = ahd::V1_1::DumpstateStatus;

const char kVerboseLoggingProperty[] = "persist.dumpstate.verbose_logging_enabled";

struct DumpstateDevice : public IDumpstateDevice1 {
    // 1.1
    Return<DumpstateStatus1> dumpstateBoard_1_1(const hidl_handle& handle,
                                                const DumpstateMode1 mode,
                                                uint64_t /*timeoutMillis*/) override {
        if (handle == nullptr || handle->numFds < 1) {
            ALOGE("no FDs\n");
            return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }

        int fd = handle->data[0];
        if (fd < 0) {
            ALOGE("invalid FD: %d\n", fd);
            return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }

        switch (mode) {
            case DumpstateMode1::FULL:
                return dumpstateBoardImpl(fd, true);

            case DumpstateMode1::DEFAULT:
                return dumpstateBoardImpl(fd, false);

            case DumpstateMode1::INTERACTIVE:
            case DumpstateMode1::REMOTE:
            case DumpstateMode1::WEAR:
            case DumpstateMode1::CONNECTIVITY:
            case DumpstateMode1::WIFI:
            case DumpstateMode1::PROTO:
                ALOGE("The requested mode is not supported: %d\n", static_cast<int>(mode));
                return DumpstateStatus1::UNSUPPORTED_MODE;

            default:
                ALOGE("The requested mode is invalid: %d\n", static_cast<int>(mode));
                return DumpstateStatus1::ILLEGAL_ARGUMENT;
        }
    }

    Return<void> setVerboseLoggingEnabled(bool enable) override {
        ::android::base::SetProperty(kVerboseLoggingProperty, enable ? "true" : "false");
        return Void();
    }

    Return<bool> getVerboseLoggingEnabled() override { return getVerboseLoggingEnabledImpl(); }

    // 1.0
    Return<void> dumpstateBoard(const hidl_handle& h) override {
        dumpstateBoard_1_1(h, DumpstateMode1::DEFAULT, 0);
        return Void();
    }

    DumpstateStatus1 dumpstateBoardImpl(const int fd, const bool full) {
        ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);
        ALOGI("Dumpstate HIDL not provided by device\n");

        if (getVerboseLoggingEnabledImpl()) {
            dprintf(fd, "%s", "verbose: ");
        }

        if (full) {
            dprintf(fd, "%s", "Hello, world!");
        } else {
            dprintf(fd, "%s", "test");
        }

        return DumpstateStatus1::OK;
    }

    static bool getVerboseLoggingEnabledImpl() {
        return ::android::base::GetBoolProperty(kVerboseLoggingProperty, false);
    }
};
}  // namespace

int main(int, char**) {
    using ::android::OK;
    using ::android::sp;
    using ::android::hardware::configureRpcThreadpool;
    using ::android::hardware::joinRpcThreadpool;

    sp<IDumpstateDevice1> dumpstate(new DumpstateDevice);

    // This method MUST be called before interacting with any HIDL interfaces.
    configureRpcThreadpool(1, true);

    if (dumpstate->registerAsService() != OK) {
        ALOGE("Could not register service.");
        return 1;
    }

    joinRpcThreadpool();
    return 0;
}
