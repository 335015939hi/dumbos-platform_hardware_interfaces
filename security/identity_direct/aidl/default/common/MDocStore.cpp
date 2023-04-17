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

#include "MDocStore.h"

namespace aidl::android::security::identity::direct_access {

ndk::ScopedAStatus MDocStore::getNumberOfCredentialSlots(int32_t* out_count) {
    int32_t ret = mProvisioningProxy->getNumberOfCredentialSlots(out_count);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocStore::createMDocCredential(int32_t in_credentialSlot,
                                                   bool in_testCredential,
                                                   const std::vector<uint8_t>& in_challenge,
                                                   std::vector<Certificate>* out_certificate) {
    int32_t ret = mProvisioningProxy->createCredential(in_credentialSlot, in_testCredential,
                                                       in_challenge, out_certificate);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocStore::lookupMDocCredential(
        int32_t in_credentialSlot, std::shared_ptr<IMDocCredential>* out_IMDocCredential) {
    int32_t ret = mProvisioningProxy->lookupMDocCredential(in_credentialSlot);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }
    *out_IMDocCredential =
            ndk::SharedRefBase::make<MDocCredential>(mProvisioningProxy, in_credentialSlot);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocStore::deleteMDocCredential(int32_t in_credentialSlot) {
    int32_t ret = mProvisioningProxy->deleteCredential(in_credentialSlot);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MDocStore::getMaximumCredentialDataSize(int64_t* out_dataSize) {
    int32_t ret = mProvisioningProxy->getMaximumCredentialDataSize(out_dataSize);
    if (ret != IMDocStore::STATUS_OK) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificError(ret));
    }
    return ndk::ScopedAStatus::ok();
}
}  // namespace aidl::android::security::identity::direct_access
