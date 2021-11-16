/*
 * Copyright (C) 2021 The Android Open Source Project
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
#include <log/log.h>
#include "DumpstateUtil.h"

#include "Dumpstate.h"

namespace aidl {
namespace android {
namespace hardware {
namespace dumpstate {

const char kVerboseLoggingProperty[] = "persist.dumpstate.verbose_logging.enabled";

ndk::ScopedAStatus Dumpstate::dumpstateBoard(const std::vector<::ndk::ScopedFileDescriptor>& in_fds,
                                             IDumpstateDevice::DumpstateMode in_mode,
                                             int64_t in_timeoutMillis) {
    (void)in_timeoutMillis;

    if (in_fds.size() < 1) {
        ALOGE("no FDs\n");
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    int fd = in_fds[0].get();
    if (fd < 0) {
        ALOGE("invalid FD: %d\n", fd);
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    binder_exception_t return_exception_code = EX_NONE;
    switch (in_mode) {
        case IDumpstateDevice::DumpstateMode::FULL:
            return_exception_code = dumpstateBoardImpl(fd, true);
            break;

        case IDumpstateDevice::DumpstateMode::DEFAULT:
            return_exception_code = dumpstateBoardImpl(fd, false);
            break;

        case IDumpstateDevice::DumpstateMode::INTERACTIVE:
        case IDumpstateDevice::DumpstateMode::REMOTE:
        case IDumpstateDevice::DumpstateMode::WEAR:
        case IDumpstateDevice::DumpstateMode::CONNECTIVITY:
        case IDumpstateDevice::DumpstateMode::WIFI:
        case IDumpstateDevice::DumpstateMode::PROTO:
            ALOGE("The requested mode is not supported: %s\n", toString(in_mode).c_str());
            return_exception_code = IDumpstateDevice::ERROR_UNSUPPORTED_MODE;
            break;

        default:
            ALOGE("The requested mode is invalid: %s\n", toString(in_mode).c_str());
            return_exception_code = EX_ILLEGAL_ARGUMENT;
            break;
    }

    if (return_exception_code == IDumpstateDevice::ERROR_UNSUPPORTED_MODE) {
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(return_exception_code,
                                                                       "UNSUPPORTED MODE");
    }
    if (return_exception_code == IDumpstateDevice::ERROR_DEVICE_LOGGING_NOT_ENABLED) {
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                return_exception_code, "DEVICE LOGGING NOT ENABLED");
    }
    return ndk::ScopedAStatus::fromExceptionCode(return_exception_code);
}

ndk::ScopedAStatus Dumpstate::getVerboseLoggingEnabled(bool* _aidl_return) {
    *_aidl_return = getVerboseLoggingEnabledImpl();

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Dumpstate::setVerboseLoggingEnabled(bool in_enable) {
    ::android::base::SetProperty(kVerboseLoggingProperty, in_enable ? "true" : "false");

    return ndk::ScopedAStatus::ok();
}

bool Dumpstate::getVerboseLoggingEnabledImpl() {
    return ::android::base::GetBoolProperty(kVerboseLoggingProperty, false);
}

binder_exception_t Dumpstate::dumpstateBoardImpl(const int fd, const bool full) {
    ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);

    dprintf(fd, "verbose logging: %s\n", getVerboseLoggingEnabledImpl() ? "enabled" : "disabled");

    dprintf(fd, "[%s] %s\n", (full ? "full" : "default"), "Hello, world!");

    return EX_NONE;
}

}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
}  // namespace aidl
