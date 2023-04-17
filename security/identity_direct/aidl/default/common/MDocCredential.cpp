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
#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <aidl/android/security/identity/direct_access/BnMDocStore.h>

#include "MDocCredential.h"

namespace aidl::android::security::identity::direct_access {

ndk::ScopedAStatus MDocCredential::presentationPackageGenerate(
        int64_t in_validityPeriodMillis, MDocPresentationPackage* out_MDocPresentationPackage) {
    std::pair<std::vector<uint8_t>, std::vector<uint8_t>> presentationPackage;
    int32_t ret = mProvisioningProxy->presentationPackageGenerate(
            mSlotNumber, in_validityPeriodMillis, time(nullptr), &presentationPackage);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }

    *out_MDocPresentationPackage = MDocPresentationPackage();
    out_MDocPresentationPackage->encryptedData = std::move(presentationPackage.second);
    out_MDocPresentationPackage->signingKeyCertificate.encodedCertificate =
            std::move(presentationPackage.first);

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::presentationPackageSetData(
        const MDocPresentationPackage& in_presentationPackage,
        const std::vector<uint8_t>& in_credentialData,
        MDocPresentationPackage* out_MDocPresentationPackage) {
    std::vector<uint8_t> encryptedData;
    int32_t ret = mProvisioningProxy->getEncryptedCredData(
            mSlotNumber, in_presentationPackage.encryptedData, in_credentialData, &encryptedData);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error while getting encryptedData"));
    }

    *out_MDocPresentationPackage = MDocPresentationPackage();
    out_MDocPresentationPackage->encryptedData = std::move(encryptedData);
    out_MDocPresentationPackage->signingKeyCertificate =
            in_presentationPackage.signingKeyCertificate;

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::currentPresentationPackageGet(Certificate* out_Certificate) {
    std::vector<uint8_t> certs;
    int32_t ret = mProvisioningProxy->getCurrentSigningCertificate(
            mSlotNumber, &out_Certificate->encodedCertificate);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error while getting certificates"));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::currentPresentationPackageSet(
        const MDocPresentationPackage& in_presentationPackage) {
    int32_t ret = mProvisioningProxy->currentPresentationPackageSet(
            mSlotNumber, in_presentationPackage.signingKeyCertificate.encodedCertificate,
            in_presentationPackage.encryptedData);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error while setting Presentation package"));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::currentPresentationPackageClear() {
    auto ret = mProvisioningProxy->clearPresentationPackage(mSlotNumber);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error while clearing presentation package"));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::currentPresentationPackageGetNumUses(int32_t* out_NumUses) {
    auto ret = mProvisioningProxy->getUsageCount(mSlotNumber, out_NumUses);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                ret, "Error while getting usage count."));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocCredential::simulatePresentation(
        const std::vector<uint8_t>& in_deviceRequestCbor,
        std::vector<uint8_t>* out_DeviceResponse) {
    (void)in_deviceRequestCbor;
    (void)out_DeviceResponse;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::security::identity::direct_access
