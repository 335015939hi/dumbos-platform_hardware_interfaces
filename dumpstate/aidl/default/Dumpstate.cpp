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

ndk::ScopedAStatus Dumpstate::dumpstateBoard(
        const std::vector<::ndk::ScopedFileDescriptor>& in_fds,
        ::aidl::android::hardware::dumpstate::DumpstateMode in_mode, int64_t in_timeoutMillis,
        ::aidl::android::hardware::dumpstate::DumpstateStatus* _aidl_return) {
    (void)in_timeoutMillis;

    *_aidl_return = DumpstateStatus::ILLEGAL_ARGUMENT;

    if (in_fds.size() < 1) {
        ALOGE("no FDs\n");
        *_aidl_return = DumpstateStatus::ILLEGAL_ARGUMENT;
        return ndk::ScopedAStatus::ok();
    }

    int fd = in_fds[0].get();
    if (fd < 0) {
        ALOGE("invalid FD: %d\n", fd);
        *_aidl_return = DumpstateStatus::ILLEGAL_ARGUMENT;
        return ndk::ScopedAStatus::ok();
    }

    switch (in_mode) {
        case DumpstateMode::FULL:
            *_aidl_return = dumpstateBoardImpl(fd, true);
            break;

        case DumpstateMode::DEFAULT:
            *_aidl_return = dumpstateBoardImpl(fd, false);
            break;

        case DumpstateMode::INTERACTIVE:
        case DumpstateMode::REMOTE:
        case DumpstateMode::WEAR:
        case DumpstateMode::CONNECTIVITY:
        case DumpstateMode::WIFI:
        case DumpstateMode::PROTO:
            ALOGE("The requested mode is not supported: %s\n", toString(in_mode).c_str());
            *_aidl_return = DumpstateStatus::UNSUPPORTED_MODE;
            break;

        default:
            ALOGE("The requested mode is invalid: %s\n", toString(in_mode).c_str());
            *_aidl_return = DumpstateStatus::ILLEGAL_ARGUMENT;
            break;
    }

    return ndk::ScopedAStatus::ok();
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

::aidl::android::hardware::dumpstate::DumpstateStatus Dumpstate::dumpstateBoardImpl(
        const int fd, const bool full) {
    ALOGD("DumpstateDevice::dumpstateBoard() FD: %d\n", fd);

    dprintf(fd, "verbose logging: %s\n", getVerboseLoggingEnabledImpl() ? "enabled" : "disabled");

    dprintf(fd, "[%s] %s\n", (full ? "full" : "default"), "Hello, world!");

    return ::aidl::android::hardware::dumpstate::DumpstateStatus::OK;
}

}  // namespace dumpstate
}  // namespace hardware
}  // namespace android
}  // namespace aidl
