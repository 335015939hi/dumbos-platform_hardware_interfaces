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

#pragma once

#include <memory>

#include <android/binder_auto_utils.h>

namespace aidl::android::hardware::health {

// Wrapper over LinkedCallback::OnCallbackDied.
// This type is hidden from the public headers so that BinderHealth::death_recipient() does not
// expose the internal ScopedAIBinder_DeathRecipient to external clients.
class DeathRecipient {
  public:
    DeathRecipient();
    AIBinder_DeathRecipient* get() const { return value_.get(); }

  private:
    ndk::ScopedAIBinder_DeathRecipient value_;
};

}  // namespace aidl::android::hardware::health
