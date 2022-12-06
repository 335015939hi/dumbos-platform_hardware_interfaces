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

#include <numeric>
#include <string>

#include <android-base/logging.h>
#include <android-base/strings.h>

#include "Offload.h"

namespace aidl::android::hardware::tetheroffload::impl::example {

using ::android::base::Join;

ndk::ScopedAStatus Offload::addDownstream(const std::string& in_iface,
                                          const std::string& in_prefix) {
    LOG(VERBOSE) << __func__ << " Downstream: " << in_iface << ", Prefix: " << in_prefix;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::getForwardedStats(const std::string& in_upstream,
                                              ForwardedStats* _aidl_return) {
    LOG(VERBOSE) << __func__ << " Upstream: " << in_upstream;
    ForwardedStats stats;
    stats.rxBytes = 1920;
    stats.txBytes = 1080;
    *_aidl_return = std::move(stats);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::initOffload(const std::shared_ptr<ITetheringOffloadCallback>& in_cb) {
    LOG(VERBOSE) << __func__ << " ITetheringOffloadCallback: " << in_cb;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::removeDownstream(const std::string& in_iface,
                                             const std::string& in_prefix) {
    LOG(VERBOSE) << __func__ << " Downstream: " << in_iface << ", Prefix: " << in_prefix;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setDataWarningAndLimit(const std::string& in_upstream,
                                                   int64_t in_warningBytes, int64_t in_limitBytes) {
    LOG(VERBOSE) << __func__ << " Upstream: " << in_upstream
                 << ", WarningBytes : " << in_warningBytes << ", LimitBytes : " << in_limitBytes;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setHandles(const ndk::ScopedFileDescriptor& in_fd1,
                                       const ndk::ScopedFileDescriptor& in_fd2) {
    LOG(VERBOSE) << __func__ << " FileDescriptor1 : " << std::to_string(in_fd1.get())
                 << ", FileDescriptor2 : " << std::to_string(in_fd2.get());
    int fd1 = in_fd1.get();
    int fd2 = in_fd2.get();
    if (fd1 < 0 || fd2 < 0) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "Invalid file descriptor");
    }
    mFd1 = ndk::ScopedFileDescriptor(dup(fd1));
    mFd2 = ndk::ScopedFileDescriptor(dup(fd2));
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setLocalPrefixes(const std::vector<std::string>& in_prefixes) {
    LOG(VERBOSE) << __func__ << " Prefixes : " << Join(in_prefixes, ',');
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::setUpstreamParameters(const std::string& in_iface,
                                                  const std::string& in_v4Addr,
                                                  const std::string& in_v4Gw,
                                                  const std::vector<std::string>& in_v6Gws) {
    LOG(VERBOSE) << __func__ << " Upstream : " << in_iface << ", IPv4Address : " << in_v4Addr
                 << ", IPv4Gateway : " << in_v4Gw << ", IPv6Gateways : " << Join(in_v6Gws, ',');
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Offload::stopOffload() {
    LOG(VERBOSE) << __func__;
    return ndk::ScopedAStatus::ok();
};

}  // namespace aidl::android::hardware::tetheroffload::impl::example
