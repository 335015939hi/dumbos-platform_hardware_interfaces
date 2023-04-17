/*
 * Copyright 2023 The Android Open Source Project
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
#ifndef ANDROID_HARDWARE_DIRECT_ACCESS_STORE_MDOC_CREDENTIAL_H
#define ANDROID_HARDWARE_DIRECT_ACCESS_STORE_MDOC_CREDENTIAL_H

#include <aidl/android/security/identity/direct_access/BnMDocCredential.h>

#include "SecureHardwareDirectAccessProvisioningProxy.h"

namespace aidl::android::security::identity::direct_access {
using aidl::android::hardware::identity::Certificate;

class MDocCredential : public BnMDocCredential {
  public:
    MDocCredential(std::shared_ptr<SecureHardwareDirectAccessProvisioningProxy> proxy,
                   uint8_t in_slotNumber)
        : mProvisioningProxy(proxy), mSlotNumber(in_slotNumber) {}

    ndk::ScopedAStatus presentationPackageGenerate(
            int64_t in_validityPeriodMillis,
            MDocPresentationPackage* out_MDocPresentationPackage) override;

    ndk::ScopedAStatus presentationPackageSetData(
            const MDocPresentationPackage& in_presentationPackage,
            const std::vector<uint8_t>& in_credentialData,
            MDocPresentationPackage* out_MDocPresentationPackage) override;

    ndk::ScopedAStatus currentPresentationPackageGet(Certificate* out_Certificate) override;

    ndk::ScopedAStatus currentPresentationPackageSet(
            const MDocPresentationPackage& in_presentationPackage) override;

    ndk::ScopedAStatus currentPresentationPackageClear() override;

    ndk::ScopedAStatus currentPresentationPackageGetNumUses(int32_t* out_NumUses) override;

    ndk::ScopedAStatus simulatePresentation(const std::vector<uint8_t>& in_deviceRequestCbor,
                                            std::vector<uint8_t>* out_DeviceResponse) override;

    std::optional<std::vector<std::optional<Certificate>>> getCredentialKeyCert();

  private:
    std::shared_ptr<SecureHardwareDirectAccessProvisioningProxy> mProvisioningProxy;
    uint8_t mSlotNumber;
};
}  // namespace aidl::android::security::identity::direct_access
#endif