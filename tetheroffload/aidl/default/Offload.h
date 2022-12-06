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

#pragma once

#include <aidl/android/hardware/tetheroffload/BnOffload.h>

namespace aidl {
namespace android {
namespace hardware {
namespace tetheroffload {
namespace impl {
namespace example {

using aidl::android::hardware::tetheroffload::ForwardedStats;
using aidl::android::hardware::tetheroffload::ITetheringOffloadCallback;

class Offload : public BnOffload {
  public:
    ndk::ScopedAStatus addDownstream(const std::string& in_iface,
                                     const std::string& in_prefix) override;
    ndk::ScopedAStatus getForwardedStats(const std::string& in_upstream,
                                         ForwardedStats* _aidl_return) override;
    ndk::ScopedAStatus initOffload(
            const std::shared_ptr<ITetheringOffloadCallback>& in_cb) override;
    ndk::ScopedAStatus removeDownstream(const std::string& in_iface,
                                        const std::string& in_prefix) override;
    ndk::ScopedAStatus setDataWarningAndLimit(const std::string& in_upstream,
                                              int64_t in_warningBytes,
                                              int64_t in_limitBytes) override;
    ndk::ScopedAStatus setHandles(const ndk::ScopedFileDescriptor& in_fd1,
                                  const ndk::ScopedFileDescriptor& in_fd2) override;
    ndk::ScopedAStatus setLocalPrefixes(const std::vector<std::string>& in_prefixes) override;
    ndk::ScopedAStatus setUpstreamParameters(const std::string& in_iface,
                                             const std::string& in_v4Addr,
                                             const std::string& in_v4Gw,
                                             const std::vector<std::string>& in_v6Gws) override;
    ndk::ScopedAStatus stopOffload() override;

  private:
    ndk::ScopedFileDescriptor mFd1;
    ndk::ScopedFileDescriptor mFd2;
};

}  // namespace example
}  // namespace impl
}  // namespace tetheroffload
}  // namespace hardware
}  // namespace android
}  // namespace aidl
