/*
 * Copyright (C) 2023 The Android Open Source Project
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

#define LOG_TAG "TestDirectAccessTests"

#include <future>
#include <map>
#include <utility>

#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <android-base/logging.h>
#include <android/binder_manager.h>

#include <aidl/android/hardware/identity/Certificate.h>
#include <aidl/android/security/identity/direct_access/BnMDocCredential.h>
#include <aidl/android/security/identity/direct_access/BnMDocStore.h>
#include <android/hardware/identity/support/IdentityCredentialSupport.h>
#include <cppbor.h>
#include <cppbor_parse.h>

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gtest/gtest.h>

namespace aidl::android::security::identity::direct_access::test {

using ::aidl::android::hardware::identity::Certificate;
using ::aidl::android::security::identity::direct_access::IMDocCredential;
using ::aidl::android::security::identity::direct_access::IMDocStore;
using ::aidl::android::security::identity::direct_access::MDocPresentationPackage;
using std::make_pair;
using std::map;
using std::optional;
using std::pair;
using std::string;
using std::tie;
using std::vector;
using Status = ::ndk::ScopedAStatus;

#define INSTANTIATE_IDENTITY_DIRECT_AIDL_TEST(name)                                        \
    INSTANTIATE_TEST_SUITE_P(                                                              \
            PerInstance, name,                                                             \
            testing::ValuesIn(::android::getAidlHalInstanceNames(IMDocStore::descriptor)), \
            ::android::PrintInstanceNameToString);                                         \
    GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(name);

class TestDireactAccessTests : public testing::TestWithParam<string> {
  public:
    void SetUp() {
        ::ndk::SpAIBinder binder(AServiceManager_waitForService(GetParam().c_str()));
        mDocStore_ = IMDocStore::fromBinder(binder);
        ASSERT_NE(mDocStore_, nullptr);
    }

    void createMDocCredential(int32_t slot) {
        string sChallenge = "test_challenge";
        vector<uint8_t> challenge;
        std::vector<Certificate> out_certificate;
        challenge.assign(sChallenge.begin(), sChallenge.end());

        ASSERT_TRUE(
                mDocStore_->createMDocCredential(slot, true, challenge, &out_certificate).isOk());
        ASSERT_GE(out_certificate.size(), 1);
    }

  protected:
    std::shared_ptr<IMDocStore> mDocStore_;
};

TEST_P(TestDireactAccessTests, SimpleDirectAccessFlowTest) {
    int32_t slotNu = 0, totalNuSlot, usageCnt;
    int64_t maxCredDataSize;
    MDocPresentationPackage mPackage;

    ASSERT_TRUE(mDocStore_->getNumberOfCredentialSlots(&totalNuSlot).isOk());
    ASSERT_GE(totalNuSlot, 1);
    ASSERT_TRUE(mDocStore_->getMaximumCredentialDataSize(&maxCredDataSize).isOk());
    ASSERT_LE(maxCredDataSize, 32768);

    createMDocCredential(slotNu);

    std::shared_ptr<IMDocCredential> mDocCredential;
    ASSERT_TRUE(mDocStore_->lookupMDocCredential(slotNu, &mDocCredential).isOk());

    // generate presentation package
    ASSERT_TRUE(mDocCredential->presentationPackageGenerate(86400000 /*1 day */, &mPackage).isOk());
    ASSERT_NE(mPackage.encryptedData.size(), 0);
    // TODO verify the certificate and it should be signed by the Credential Key.

    // Set CredData dummy TODO need to change as per type
    std::vector<uint8_t> nil(5, 5);
    cppbor::Map credData = cppbor::Map()
                                   .add("docType", nil)
                                   .add("digestIdMapping", nil)
                                   .add("issuerAuth", nil)
                                   .add("readerAccess", nil);
    vector<uint8_t> credDataVec = credData.encode();

    MDocPresentationPackage mPackageTemp;
    ASSERT_TRUE(mDocCredential->presentationPackageSetData(mPackage, credDataVec, &mPackageTemp)
                        .isOk());

    ASSERT_NE(mPackage.encryptedData.size(), 0);
    // TODO verify the certificate

    // set mPackageTemp as current data
    ASSERT_TRUE(mDocCredential->currentPresentationPackageSet(mPackageTemp).isOk());

    // get current signing certificate
    Certificate certificate;
    ASSERT_TRUE(mDocCredential->currentPresentationPackageGet(&certificate).isOk());

    // verify the certificate
    // ASSERT_NE(certificate, mPackage.signingKeyCertificate);
    ASSERT_EQ(0, memcmp(certificate.encodedCertificate.data(),
                        mPackage.signingKeyCertificate.encodedCertificate.data(),
                        certificate.encodedCertificate.size()));

    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_EQ(usageCnt, 0);

    // TODO request and response implementation
    std::vector<uint8_t> in_deviceRequestCbor, out_DeviceResponse;
    ASSERT_TRUE(
            mDocCredential->simulatePresentation(in_deviceRequestCbor, &out_DeviceResponse).isOk());

    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_NE(usageCnt, 1);

    // reset the usage count
    ASSERT_TRUE(mDocCredential->currentPresentationPackageClear().isOk());
    ASSERT_TRUE(mDocCredential->currentPresentationPackageGetNumUses(&usageCnt).isOk());
    ASSERT_EQ(usageCnt, 0);

    // delete MDocCredential
    ASSERT_TRUE(mDocStore_->deleteMDocCredential(slotNu).isOk());
}

INSTANTIATE_IDENTITY_DIRECT_AIDL_TEST(TestDireactAccessTests);

}  // namespace aidl::android::security::identity::direct_access::test
