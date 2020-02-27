/*
 * Copyright 2019, The Android Open Source Project
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

#define LOG_TAG "WritableIdentityCredential"

#include "WritableIdentityCredential.h"

#include <android/hardware/identity/support/IdentityCredentialSupport.h>

#include <android-base/logging.h>

#include <cppbor/cppbor.h>
#include <cppbor/cppbor_parse.h>

#include <utility>

#include "IdentityCredentialStore.h"
#include "Util.h"

#include "FakeSecureHardwareProxy.h"

namespace aidl::android::hardware::identity {

using ::std::optional;
using namespace ::android::hardware::identity;

bool WritableIdentityCredential::initialize() {

    hwProxy_ = new FakeSecureHardwareProvisioningProxy();
    if (!hwProxy_->initialize(testCredential_)) {
        LOG(ERROR) << "shwProxy->initialize failed";
        return false;
    }

    return true;
}

WritableIdentityCredential::~WritableIdentityCredential() {}

// Helper to get hard-coded attestation certificates.
static bool appendAttestationCerts(vector<Certificate>& certs) {
    const int secondsInOneYear = 365 * 24 * 60 * 60;
    time_t validityNotBefore = time(nullptr); // now
    time_t validityNotAfter = validityNotBefore + 10 * secondsInOneYear; // Ten years from now.

    uint8_t rootPub[EIC_P256_PUB_KEY_SIZE];
    uint8_t rootPriv[EIC_P256_PRIV_KEY_SIZE];
    if (!eicOpsCreateEcKey(rootPriv, rootPub)) {
        LOG(ERROR) << "Error creating root key";
        return false;
    }
    uint8_t rootKeyCert[512];
    size_t rootKeyCertSize = sizeof(rootKeyCert);
    if (!eicOpsSignEcKey(rootPub,
                         rootPriv,
                         1,
                         "TODO: issuer",
                         "TODO: subject",
                         validityNotBefore,
                         validityNotAfter,
                         rootKeyCert,
                         &rootKeyCertSize)) {
        LOG(ERROR) << "Error self-signing root certificate";
        return false;
    }
    Certificate rootCert;
    rootCert.encodedCertificate.resize(rootKeyCertSize);
    memcpy(rootCert.encodedCertificate.data(), rootKeyCert, rootKeyCertSize);

    uint8_t attestationKeyCert[512];
    size_t attestationKeyCertSize = sizeof(attestationKeyCert);
    const uint8_t* attestationPub = eicOpsGetAttestationPublicKey();
    if (!eicOpsSignEcKey(attestationPub,
                         rootPriv,
                         1,
                         "TODO: issuer",
                         "TODO: subject",
                         validityNotBefore,
                         validityNotAfter,
                         attestationKeyCert,
                         &attestationKeyCertSize)) {
        LOG(ERROR) << "Error signing attestation certificate with root private key";
        return false;
    }
    Certificate attestationCert;
    attestationCert.encodedCertificate.resize(attestationKeyCertSize);
    memcpy(attestationCert.encodedCertificate.data(), attestationKeyCert, attestationKeyCertSize);

    // The root certificate goes last.
    certs.push_back(attestationCert);
    certs.push_back(rootCert);

    return true;
}

ndk::ScopedAStatus WritableIdentityCredential::getAttestationCertificate(
        const vector<uint8_t>& attestationApplicationId,
        const vector<uint8_t>& attestationChallenge,
        vector<Certificate>* outCertificateChain) {

    if (getAttestationCertificateAlreadyCalled_) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED,
                "Error attestation certificate previously generated"));
    }
    getAttestationCertificateAlreadyCalled_ = true;

    optional<vector<uint8_t>> pubKeyCert = hwProxy_->createCredentialKey(
        attestationChallenge,
        attestationApplicationId);
    if (!pubKeyCert) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
            IIdentityCredentialStore::STATUS_FAILED,
            "Error generating attestation certificate"));
    }

    optional<vector<uint8_t>> pubKey = support::certificateChainGetTopMostKey(pubKeyCert.value());
    if (!pubKey) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
            IIdentityCredentialStore::STATUS_FAILED,
            "Error extracting public key from certificate"));
    }

    *outCertificateChain = vector<Certificate>(0);
    Certificate c = Certificate();
    c.encodedCertificate = pubKeyCert.value();
    outCertificateChain->push_back(std::move(c));

    if (!appendAttestationCerts(*outCertificateChain)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
            IIdentityCredentialStore::STATUS_FAILED, "Error appending attestation certificates"));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::startPersonalization(
    int32_t accessControlProfileCount, const vector<int32_t>& entryCounts,
    int32_t expectedProofOfProvisioningSize) {
    numAccessControlProfileRemaining_ = accessControlProfileCount;
    remainingEntryCounts_ = entryCounts;
    entryNameSpace_ = "";

    signedDataAccessControlProfiles_ = cppbor::Array();
    signedDataNamespaces_ = cppbor::Map();
    signedDataCurrentNamespace_ = cppbor::Array();

    if (!hwProxy_->startPersonalization(accessControlProfileCount,
                                        entryCounts,
                                        docType_,
                                        expectedProofOfProvisioningSize)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "eicStartPersonalization"));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::addAccessControlProfile(
        int32_t id, const Certificate& readerCertificate, bool userAuthenticationRequired,
        int64_t timeoutMillis, int64_t secureUserId,
        SecureAccessControlProfile* outSecureAccessControlProfile) {

    if (numAccessControlProfileRemaining_ == 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "numAccessControlProfileRemaining_ is 0 and expected non-zero"));
    }

    // Spec requires if |userAuthenticationRequired| is false, then |timeoutMillis| must also
    // be zero.
    if (!userAuthenticationRequired && timeoutMillis != 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "userAuthenticationRequired is false but timeout is non-zero"));
    }

    optional<vector<uint8_t>> mac = hwProxy_->addAccessControlProfile(
        id, readerCertificate.encodedCertificate,
        userAuthenticationRequired,
        timeoutMillis,
        secureUserId);
    if (!mac) {
      return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
          IIdentityCredentialStore::STATUS_FAILED, "eicAddAccessControlProfile"));
    }

    SecureAccessControlProfile profile;
    profile.id = id;
    profile.readerCertificate = readerCertificate;
    profile.userAuthenticationRequired = userAuthenticationRequired;
    profile.timeoutMillis = timeoutMillis;
    profile.secureUserId = secureUserId;
    profile.mac = mac.value();

    cppbor::Map profileMap;
    profileMap.add("id", profile.id);
    if (profile.readerCertificate.encodedCertificate.size() > 0) {
        profileMap.add("readerCertificate",
                       cppbor::Bstr(profile.readerCertificate.encodedCertificate));
    }
    if (profile.userAuthenticationRequired) {
        profileMap.add("userAuthenticationRequired", profile.userAuthenticationRequired);
        profileMap.add("timeoutMillis", profile.timeoutMillis);
    }
    signedDataAccessControlProfiles_.add(std::move(profileMap));

    numAccessControlProfileRemaining_--;

    *outSecureAccessControlProfile = profile;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::beginAddEntry(
        const vector<int32_t>& accessControlProfileIds, const string& nameSpace, const string& name,
        int32_t entrySize) {
    if (numAccessControlProfileRemaining_ != 0) {
        LOG(ERROR) << "numAccessControlProfileRemaining_ is " << numAccessControlProfileRemaining_
                   << " and expected zero";
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "numAccessControlProfileRemaining_ is not zero"));
    }

    if (remainingEntryCounts_.size() == 0) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA, "No more namespaces to add to"));
    }

    // Handle initial beginEntry() call.
    if (entryNameSpace_ == "") {
        entryNameSpace_ = nameSpace;
    }

    // If the namespace changed...
    if (nameSpace != entryNameSpace_) {
        // Then check that all entries in the previous namespace have been added..
        if (remainingEntryCounts_[0] != 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "New namespace but a non-zero number of entries remain to be added"));
        }
        remainingEntryCounts_.erase(remainingEntryCounts_.begin());

        if (signedDataCurrentNamespace_.size() > 0) {
            signedDataNamespaces_.add(entryNameSpace_, std::move(signedDataCurrentNamespace_));
            signedDataCurrentNamespace_ = cppbor::Array();
        }
    } else {
        // Same namespace...
        if (remainingEntryCounts_[0] == 0) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Same namespace but no entries remain to be added"));
        }
        remainingEntryCounts_[0] -= 1;
    }

    entryAdditionalData_ = entryCreateAdditionalData(nameSpace, name, accessControlProfileIds);

    entryRemainingBytes_ = entrySize;
    entryNameSpace_ = nameSpace;
    entryName_ = name;
    entryAccessControlProfileIds_ = accessControlProfileIds;
    entryBytes_.resize(0);
    // LOG(INFO) << "name=" << name << " entrySize=" << entrySize;

    if (!hwProxy_->beginAddEntry(accessControlProfileIds,
                                 nameSpace,
                                 name,
                                 entrySize)) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "eicBeginAddEntry"));
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::addEntryValue(const vector<uint8_t>& content,
                                                             vector<uint8_t>* outEncryptedContent) {
    size_t contentSize = content.size();

    if (contentSize > IdentityCredentialStore::kGcmChunkSize) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "Passed in chunk of is bigger than kGcmChunkSize"));
    }
    if (contentSize > entryRemainingBytes_) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_INVALID_DATA,
                "Passed in chunk is bigger than remaining space"));
    }

    entryBytes_.insert(entryBytes_.end(), content.begin(), content.end());
    entryRemainingBytes_ -= contentSize;
    if (entryRemainingBytes_ > 0) {
        if (contentSize != IdentityCredentialStore::kGcmChunkSize) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA,
                    "Retrieved non-final chunk which isn't kGcmChunkSize"));
        }
    }

    optional<vector<uint8_t>> encryptedContent = hwProxy_->addEntryValue(
        entryAccessControlProfileIds_,
        entryNameSpace_,
        entryName_,
        content);
    if (!encryptedContent) {
        return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                IIdentityCredentialStore::STATUS_FAILED, "eicAddEntryValue"));
    }

    if (entryRemainingBytes_ == 0) {
        // TODO: ideally do do this without parsing the data (but still validate data is valid
        // CBOR).
        auto [item, _, message] = cppbor::parse(entryBytes_);
        if (item == nullptr) {
            return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
                    IIdentityCredentialStore::STATUS_INVALID_DATA, "Data is not valid CBOR"));
        }
        cppbor::Map entryMap;
        entryMap.add("name", entryName_);
        entryMap.add("value", std::move(item));
        cppbor::Array profileIdArray;
        for (auto id : entryAccessControlProfileIds_) {
            profileIdArray.add(id);
        }
        entryMap.add("accessControlProfiles", std::move(profileIdArray));
        signedDataCurrentNamespace_.add(std::move(entryMap));
    }

    *outEncryptedContent = encryptedContent.value();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus WritableIdentityCredential::finishAddingEntries(
        vector<uint8_t>* outCredentialData, vector<uint8_t>* outProofOfProvisioningSignature) {
    if (signedDataCurrentNamespace_.size() > 0) {
        signedDataNamespaces_.add(entryNameSpace_, std::move(signedDataCurrentNamespace_));
    }
    cppbor::Array popArray;
    popArray.add("ProofOfProvisioning")
            .add(docType_)
            .add(std::move(signedDataAccessControlProfiles_))
            .add(std::move(signedDataNamespaces_))
            .add(testCredential_);
    vector<uint8_t> encodedCbor = popArray.encode();

    optional<vector<uint8_t>> signatureOfToBeSigned =
        hwProxy_->finishAddingEntries(testCredential_);
    if (!signatureOfToBeSigned) {
      return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
          IIdentityCredentialStore::STATUS_FAILED, "eicFinishAddingEntries"));
    }

    optional<vector<uint8_t>> signature = support::coseSignEcDsaWithSignature(
        signatureOfToBeSigned.value(),
        encodedCbor,  // data
        {});          // certificateChain
    if (!signature) {
      return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
          IIdentityCredentialStore::STATUS_FAILED, "Error signing data"));
    }

    optional<vector<uint8_t>> encryptedCredentialKeys =
        hwProxy_->finishGetCredentialData(testCredential_, docType_);
    if (!encryptedCredentialKeys) {
      return ndk::ScopedAStatus(AStatus_fromServiceSpecificErrorWithMessage(
          IIdentityCredentialStore::STATUS_FAILED, "Error generating encrypted CredentialKeys"));
    }
    cppbor::Array array;
    array.add(docType_);
    array.add(testCredential_);
    array.add(encryptedCredentialKeys.value());
    vector<uint8_t> credentialData = array.encode();

    *outCredentialData = credentialData;
    *outProofOfProvisioningSignature = signature.value();

    hwProxy_->shutdown();

    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::identity
