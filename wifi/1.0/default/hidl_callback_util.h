/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef HIDL_CALLBACK_UTIL_H_
#define HIDL_CALLBACK_UTIL_H_

#include <hidl/HidlSupport.h>

// Provides a class to manage callbacks for the various HIDL interfaces and
// handle the death of the process hosting each callback.
namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {
namespace hidl_callback_util {
// This class is a |RefBase| derived class and hence needs to be handled by a
// smart pointer (android::sp or android::wp).
template <typename CallbackType>
class HidlCallbackHandler : public hidl_death_recipient {
 public:
  HidlCallbackHandler() = default;
  ~HidlCallbackHandler() = default;

  bool addCallback(const sp<CallbackType>& cb) {
    // TODO(b/33818800): Can't compare proxies yet. So, use the cookie
    // (callback proxy's raw pointer) to track the death of individual clients.
    // This is guaranteed to be unique since the raw pointer of the proxy will
    // only be freed when we remove this callback from |cb_map_|.
    uint64_t cookie = reinterpret_cast<uint64_t>(cb.get());
    if (cb_map_.find(cookie) != cb_map_.end()) {
      LOG(WARNING) << "Duplicate death notification registration";
      return true;
    }
    if (!cb->linkToDeath(this, cookie)) {
      LOG(ERROR) << "Failed to register death notification";
      return false;
    }
    cb_map_[cookie] = cb;
    return true;
  }

  const std::vector<android::sp<CallbackType>> getCallbacks() {
    std::vector<android::sp<CallbackType>> cbs;
    for (const auto& iter : cb_map_) {
      cbs.push_back(iter.second);
    }
    return cbs;
  }

  void invalidate() {
    for (const auto& iter : cb_map_) {
      const sp<CallbackType>& cb = iter.second;
      if (!cb->unlinkToDeath(this)) {
        LOG(ERROR) << "Failed to deregister death notification";
      }
    }
    cb_map_.clear();
  }

  // Death notification for callbacks.
  void serviceDied(
      uint64_t cookie,
      const android::wp<android::hidl::base::V1_0::IBase>& /* who */) override {
    if (!cb_map_.erase(cookie)) {
      LOG(ERROR) << "Unknown callback death notification received";
      return;
    }
    LOG(DEBUG) << "Dead callback removed from list";
  }

 private:
  std::map<uint64_t, sp<CallbackType>> cb_map_;

  DISALLOW_COPY_AND_ASSIGN(HidlCallbackHandler);
};
}  // namespace hidl_callback_util
}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android
#endif  // HIDL_CALLBACK_UTIL_H_
