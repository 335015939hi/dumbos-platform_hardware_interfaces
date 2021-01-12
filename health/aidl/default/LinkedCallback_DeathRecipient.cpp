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

#include <android-base/logging.h>
#include <android/binder_ibinder.h>

#include <health-impl/DeathRecipient.h>
#include <health-impl/LinkedCallback.h>

namespace aidl::android::hardware::health {

LinkedCallback::LinkedCallback(BinderHealth* service, std::shared_ptr<IHealthInfoCallback> callback)
    : service_(service), callback_(callback) {
    binder_status_t linkRet =
            AIBinder_linkToDeath(callback_->asBinder().get(), service_->death_recipient()->get(),
                                 reinterpret_cast<void*>(this));
    if (linkRet != ::STATUS_OK) {
        LOG(WARNING) << __func__ << "Cannot link to death: " << linkRet;
        // ignore the error
    }
}

LinkedCallback::~LinkedCallback() {
    (void)AIBinder_unlinkToDeath(callback_->asBinder().get(), service_->death_recipient()->get(),
                                 reinterpret_cast<void*>(this));  // ignore errors
}

void LinkedCallback::OnCallbackDied() {
    service_->unregisterCallback(callback_);
}

namespace {
// Wrap LinkedCallback::OnCallbackDied() into a void(void*).
void OnCallbackDiedWrapped(void* cookie) {
    LinkedCallback* linked = reinterpret_cast<LinkedCallback*>(cookie);
    linked->OnCallbackDied();
}
}  // namespace

DeathRecipient::DeathRecipient()
    : value_(ndk::ScopedAIBinder_DeathRecipient(
              AIBinder_DeathRecipient_new(&OnCallbackDiedWrapped))) {}

}  // namespace aidl::android::hardware::health
