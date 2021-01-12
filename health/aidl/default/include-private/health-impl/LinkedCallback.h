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

#include <aidl/android/hardware/health/IHealthInfoCallback.h>
#include <android/binder_auto_utils.h>
#include <health-impl/BinderHealth.h>

#include "DeathRecipient.h"

namespace aidl::android::hardware::health {

// A (BinderHealth, IHealthInfoCallback) tuple.
// The life time of the BinderHealth service must be longer than this LinkedCallback object. This
// is maintained by storing LinkedCallback objects in the associated service.
class LinkedCallback {
  public:
    // Automatically linkToDeath upon construction with |this| as the cookie.
    // service->death_reciepient() should be from CreateDeathRecipient().
    LinkedCallback(BinderHealth* service, std::shared_ptr<IHealthInfoCallback> callback);

    // Automatically unlinkToDeath upon destruction. So, it is always safe to reinterpret_cast
    // the cookie back to the LinkedCallback object.
    ~LinkedCallback();

    // The wrapped IHealthInfoCallback object.
    IHealthInfoCallback* callback() const { return callback_.get(); }

    // On callback died, unreigster it from the service.
    void OnCallbackDied();

  private:
    BinderHealth* service_;
    std::shared_ptr<IHealthInfoCallback> callback_;
};

}  // namespace aidl::android::hardware::health
