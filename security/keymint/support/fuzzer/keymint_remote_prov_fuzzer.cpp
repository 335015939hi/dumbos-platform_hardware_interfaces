/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <android/binder_manager.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <remote_prov/remote_prov_utils.h>

namespace android::hardware::security::keymint_support::fuzzer {

using namespace cppcose;
using namespace aidl::android::hardware::security::keymint;
using namespace aidl::android::hardware::security::keymint::remote_prov;

constexpr size_t kMinSize = 0;
constexpr size_t kSupportedNumKeys = 4;
constexpr size_t kChallengeSize = 64;
constexpr size_t kMaxBytes = 128;
const std::string kServiceName =
        "android.hardware.security.keymint.IRemotelyProvisionedComponent/default";

class KeyMintRemoteProv {
  public:
    KeyMintRemoteProv(const uint8_t* data, size_t size) : mFdp(data, size){};
    void process();

  private:
    void ExtractPayloadValue(const MacedPublicKey& macedPubKey, std::vector<uint8_t>* payloadValue);
    std::shared_ptr<IRemotelyProvisionedComponent> mRPC = nullptr;
    FuzzedDataProvider mFdp;
};

void KeyMintRemoteProv::ExtractPayloadValue(const MacedPublicKey& macedPubKey,
                                            std::vector<uint8_t>* payloadValue) {
    auto [coseMac0, _, mac0ParseErr] = cppbor::parse(macedPubKey.macedKey);

    // The payload is a bstr holding an encoded COSE_Key
    auto payload = coseMac0->asArray()->get(kCoseMac0Payload)->asBstr();
    if (payload != nullptr) {
        auto generateHmac = [](const cppcose::bytevec& input) {
            return cppcose::generateHmacSha256(remote_prov::kTestMacKey, input);
        };
        std::vector<uint8_t> externalAad = mFdp.ConsumeBytes<uint8_t>(kMaxBytes);
        cppcose::generateCoseMac0Mac(generateHmac, externalAad, payload->value());

        if (payloadValue != nullptr) {
            *payloadValue = payload->value();
        }
    }
}

void KeyMintRemoteProv::process() {
    ::ndk::SpAIBinder binder(AServiceManager_waitForService(kServiceName.c_str()));
    mRPC = IRemotelyProvisionedComponent::fromBinder(binder);
    if (!mRPC) {
        return;
    }

    uint8_t challengeSize = mFdp.ConsumeIntegralInRange<uint8_t>(kMinSize, kChallengeSize);
    std::vector<uint8_t> challenge = mFdp.ConsumeBytes<uint8_t>(challengeSize);

    std::vector<MacedPublicKey> keysToSign = std::vector<MacedPublicKey>(
            mFdp.ConsumeIntegralInRange<uint8_t>(kMinSize, kSupportedNumKeys));
    cppbor::Array cborKeysToSign;
    for (auto& key : keysToSign) {
        std::vector<uint8_t> privateKeyBlob;
        mRPC->generateEcdsaP256KeyPair(false /* testMode */, &key, &privateKeyBlob);

        std::vector<uint8_t> payloadValue;
        ExtractPayloadValue(key, &payloadValue);
        cborKeysToSign.add(cppbor::EncodedItem(payloadValue));
    }

    std::vector<uint8_t> csr;
    mRPC->generateCertificateRequestV2(keysToSign, challenge, &csr);

    while (mFdp.remaining_bytes()) {
        auto invokeProvAPI = mFdp.PickValueInArray<const std::function<void()>>({
                [&]() {
                    std::vector<uint8_t> eekId;
                    if (mFdp.ConsumeBool()) {
                        eekId = mFdp.ConsumeBytes<uint8_t>(kMaxBytes);
                    }
                    generateEekChain(mFdp.ConsumeIntegral<uint8_t>() /* supportedEekCurve */,
                                     mFdp.ConsumeIntegral<uint8_t>() /* length */, eekId);
                },
                [&]() { getProdEekChain(mFdp.ConsumeIntegral<uint8_t>() /* supportedEekCurve */); },
                [&]() {
                    std::string serialNoProp = mFdp.ConsumeRandomLengthString(kMaxBytes);
                    std::string instanceName = mFdp.ConsumeRandomLengthString(kMaxBytes);
                    cppbor::Array array;
                    array.add(mFdp.ConsumeIntegral<uint8_t>() /* value */);
                    jsonEncodeCsrWithBuild(instanceName, array, serialNoProp);
                },
                [&]() { verifyFactoryCsr(cborKeysToSign, csr, mRPC.get(), challenge); },
                [&]() { verifyProductionCsr(cborKeysToSign, csr, mRPC.get(), challenge); },
                [&]() { isCsrWithProperDiceChain(csr); },
        });
        invokeProvAPI();
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    KeyMintRemoteProv kmRemoteProv(data, size);
    kmRemoteProv.process();
    return 0;
}

}  // namespace android::hardware::security::keymint_support::fuzzer
