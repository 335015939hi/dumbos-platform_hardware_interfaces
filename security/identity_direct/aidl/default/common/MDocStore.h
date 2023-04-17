/*
 * Copyright 2023, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <aidl/android/security/identity/direct_access/BnMDocCredential.h>
#include <aidl/android/security/identity/direct_access/BnMDocStore.h>

#include "MDocCredential.h"

namespace aidl::android::security::identity::direct_access {
using aidl::android::hardware::identity::Certificate;

class MDocStore : public BnMDocStore {
  public:
    MDocStore(std::shared_ptr<SecureHardwareDirectAccessProvisioningProxy> proxy)
        : mProvisioningProxy(proxy) {}

    ndk::ScopedAStatus getNumberOfCredentialSlots(int32_t* out_count) override;

    ndk::ScopedAStatus createMDocCredential(int32_t in_credentialSlot, bool in_testCredential,
                                            const std::vector<uint8_t>& in_challenge,
                                            std::vector<Certificate>* out_certificate) override;

    ndk::ScopedAStatus lookupMDocCredential(
            int32_t in_credentialSlot,
            std::shared_ptr<IMDocCredential>* out_IMDocCredential) override;

    ndk::ScopedAStatus deleteMDocCredential(int32_t in_credentialSlot) override;

    ndk::ScopedAStatus getMaximumCredentialDataSize(int64_t* out_dataSize) override;

  private:
    std::shared_ptr<SecureHardwareDirectAccessProvisioningProxy> mProvisioningProxy;
};

}  // namespace aidl::android::security::identity::direct_access
