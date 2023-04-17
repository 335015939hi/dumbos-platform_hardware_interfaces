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
#pragma once
#include <optional>
#include <vector>

#include <aidl/android/hardware/identity/Certificate.h>
#include <utils/RefBase.h>

namespace aidl::android::security::identity::direct_access {

using aidl::android::hardware::identity::Certificate;
using ::android::RefBase;

// The proxy used for provisioning
class SecureHardwareProxy : public RefBase {
  public:
    SecureHardwareProxy() {}
    virtual ~SecureHardwareProxy() {}

    virtual int32_t getNumberOfCredentialSlots(int32_t* slots);

    virtual int32_t createCredential(int32_t credentialSlot, bool testCredential,
                                     std::vector<uint8_t> challenge,
                                     std::vector<Certificate>* certificate);

    virtual int32_t deleteCredential(int credentialSlot);

    virtual int32_t getMaximumCredentialDataSize(int64_t* maxCredDataSize);

    virtual int32_t lookupMDocCredential(int credentialSlot);

    virtual int32_t getUsageCount(int credentialSlot, int32_t* usageCount);

    virtual int32_t presentationPackageGenerate(
            uint32_t credentialSlot, uint64_t validityPeriodMillis, uint64_t currentTime,
            std::pair<std::vector<uint8_t>, std::vector<uint8_t>>* out);

    virtual int32_t getEncryptedCredData(uint32_t credentialSlot,
                                         const std::vector<uint8_t>& in_encryptedData,
                                         const std::vector<uint8_t>& in_credentialData,
                                         std::vector<uint8_t>* out);

    virtual int32_t currentPresentationPackageSet(uint32_t credentialSlot,
                                                  const std::vector<uint8_t>& in_signingCertificate,
                                                  const std::vector<uint8_t>& in_encryptedData);

    virtual int32_t clearPresentationPackage(uint32_t credentialSlot);

    virtual int32_t getCurrentSigningCertificate(uint32_t credentialSlot,
                                                 std::vector<uint8_t>* out);
};
}  // namespace aidl::android::security::identity::direct_access