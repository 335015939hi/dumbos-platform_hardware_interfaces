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

#ifndef ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H
#define ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H

#include <android-base/scopeguard.h>
#include <android-base/thread_annotations.h>
#include <android/hidl/base/1.0/IBase.h>
#include <hidl/HidlSupport.h>
#include <nnapi/Result.h>
#include <nnapi/Types.h>

#include <functional>
#include <mutex>
#include <vector>

namespace android::hardware::neuralnetworks::utils {

class IProtectedCallback {
  public:
    IProtectedCallback() = default;
    IProtectedCallback(const IProtectedCallback&) = delete;
    IProtectedCallback(IProtectedCallback&&) noexcept = delete;
    IProtectedCallback& operator=(const IProtectedCallback&) = delete;
    IProtectedCallback& operator=(IProtectedCallback&&) noexcept = delete;
    virtual ~IProtectedCallback() = default;

    virtual void notifyAsDeadObject() = 0;
};

// Thread safe class
class DeathRecipient : public hidl_death_recipient {
  public:
    void serviceDied(uint64_t /*cookie*/, const wp<hidl::base::V1_0::IBase>& /*who*/) override;
    void add(IProtectedCallback* killable) const;
    void remove(IProtectedCallback* killable) const;

  private:
    mutable std::mutex mMutex;
    mutable std::vector<IProtectedCallback*> mObjects GUARDED_BY(mMutex);
};

class DeathHandler {
  public:
    static nn::GeneralResult<DeathHandler> create(sp<hidl::base::V1_0::IBase> object);

    DeathHandler(sp<hidl::base::V1_0::IBase> object, sp<DeathRecipient> deathRecipient);
    DeathHandler(const DeathHandler&) = delete;
    DeathHandler(DeathHandler&&) noexcept = default;
    DeathHandler& operator=(const DeathHandler&) = delete;
    DeathHandler& operator=(DeathHandler&&) noexcept = delete;
    ~DeathHandler();

    using Cleanup = std::function<void()>;
    [[nodiscard]] base::ScopeGuard<Cleanup> protectCallback(IProtectedCallback* killable) const;

  private:
    sp<hidl::base::V1_0::IBase> kObject;
    sp<DeathRecipient> kDeathRecipient;
};

}  // namespace android::hardware::neuralnetworks::utils

#endif  // ANDROID_HARDWARE_INTERFACES_NEURALNETWORKS_UTILS_COMMON_PROTECT_CALLBACK_H
