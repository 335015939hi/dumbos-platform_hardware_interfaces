/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include <android-base/logging.h>

#include "hidl_callback_util.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_callback_util {
template <typename CallbackType>
bool HidlCallbackHandler<CallbackType>::addCallback(
    const android::sp<CallbackType>& cb) {
  // TODO(b/33818800): Can't compare proxies yet. So, use the cookie to
  // store the original proxy pointer and use that to lookup the list.
  uint64_t cookie = static_cast<uint64_t>(cb);
  if (!cb.linkToDeath(this, cookie)) {
    LOG(ERROR) << "Failed to register for death notification for " << cb;
    return false;
  }
  cb_list.push_back(cb);
  return true;
}

template <typename CallbackType>
const std::vector<android::sp<CallbackType>>
HidlCallbackHandler<CallbackType>::getCallbacks() {
  return cb_list;
}

template <typename CallbackType>
void HidlCallbackHandler<CallbackType>::serviceDied(
    uint64_t cookie,
    const android::wp<android::hidl::base::V1_0::IBase>& /* who */) {
  android::sp<CallbackType> cb = static_cast<android::sp<CallbackType>>(cookie);
  const auto& iter = cb_list.find(cb);
  if (iter == cb_list.end()) {
    LOG(ERROR) << "Unknown death notification for " << cb;
    return;
  }
  LOG(DEBUG) << "Callback removed due to death " << cb;
  cb_list.erase(iter);
}
}  // namespace hidl_callback_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
