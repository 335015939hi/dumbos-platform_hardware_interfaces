/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/secure_element/BnSecureElement.h>

#include <android-base/hex.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

using aidl::android::hardware::secure_element::BnSecureElement;
using aidl::android::hardware::secure_element::ISecureElementCallback;
using aidl::android::hardware::secure_element::LogicalChannelResponse;
using android::base::HexString;
using ndk::ScopedAStatus;

static const std::vector<uint8_t> kAndroidTestAid = {
        0xA0, 0x00, 0x00, 0x04, 0x76, 0x41, 0x6E, 0x64,
        0x72, 0x6F, 0x69, 0x64, 0x43, 0x54, 0x53, 0x31,
};

static const std::vector<uint8_t> kAndroidTestSelectResponse = {
        0x00, 0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x00,
        0x01, 0x02, 0x03, 0x00, 0x01, 0x02, 0x03, 0x90, 0x00,
};

static const std::vector<uint8_t> kIssuerSecurityDomainSelectResponse = {
        0x00,
        0x00,
        0x90,
        0x00,
};

static bool isPrefix(std::vector<uint8_t> const& prefix, std::vector<uint8_t> const& vec) {
    if (prefix.size() > vec.size()) {
        return false;
    }

    for (size_t n = 0; n < prefix.size(); n++) {
        if (prefix[n] != vec[n]) {
            return false;
        }
    }

    return true;
}

class EmulatedSecureElement : public BnSecureElement {
  public:
    ScopedAStatus init(const std::shared_ptr<ISecureElementCallback>& clientCallback) override {
        LOG(INFO) << __func__ << " callback: " << clientCallback.get();
        if (!clientCallback) {
            return ScopedAStatus::fromExceptionCode(EX_NULL_POINTER);
        }
        mClientCallback = clientCallback;
        mClientCallback->onStateChange(true, "init");
        return ScopedAStatus::ok();
    }

    ScopedAStatus getAtr(std::vector<uint8_t>* aidlReturn) override {
        LOG(INFO) << __func__;
        *aidlReturn = mATR;
        return ScopedAStatus::ok();
    }

    ScopedAStatus reset() override {
        LOG(INFO) << __func__;
        mClientCallback->onStateChange(false, "reset");
        mClientCallback->onStateChange(true, "reset");
        // All channels are closed after reset.
        mBasicChannel = Channel();
        for (auto channel : mLogicalChannels) {
            channel = Channel();
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus isCardPresent(bool* aidlReturn) override {
        LOG(INFO) << __func__;
        *aidlReturn = true;
        return ScopedAStatus::ok();
    }

    ScopedAStatus openBasicChannel(const std::vector<uint8_t>& aid, int8_t p2,
                                   std::vector<uint8_t>* aidlReturn) override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        std::vector<uint8_t> selectResponse;

        // The basic channel can only be opened once, and stays opened
        // and locked until the SE is reset.
        if (mBasicChannel.opened) {
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }

        // If the AID is defined (the AID is not Null and the length of the
        // AID is not 0) and the channel is not locked then the corresponding
        // applet shall be selected.
        if (aid.size() > 0) {
            if (isPrefix(aid, kAndroidTestAid)) {
                selectResponse = kAndroidTestSelectResponse;
            } else {
                // No applet registered with matching AID.
                return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
            }
        }

        // If the AID is a 0 length AID and the channel is not locked, the
        // method will select the Issuer Security Domain of the SE by sending a
        // SELECT command with a 0 length AID as defined in
        // [GP Card specification].
        if (aid.size() == 0) {
            selectResponse = kIssuerSecurityDomainSelectResponse;
        }

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.
        mBasicChannel = Channel(aid, p2);
        *aidlReturn = selectResponse;
        return ScopedAStatus::ok();
    }

    ScopedAStatus openLogicalChannel(
            const std::vector<uint8_t>& aid, int8_t p2,
            ::aidl::android::hardware::secure_element::LogicalChannelResponse* aidlReturn)
            override {
        LOG(INFO) << __func__ << " aid: " << HexString(aid.data(), aid.size()) << " (" << aid.size()
                  << ") p2 " << p2;

        int8_t channelNumber = 0;
        std::vector<uint8_t> selectResponse;

        // Look for an available channel number.
        for (; channelNumber < mLogicalChannels.size(); channelNumber++) {
            if (mLogicalChannels[channelNumber].opened == false) {
                break;
            }
        }

        // All channels are currently allocated.
        if (channelNumber >= mLogicalChannels.size()) {
            return ScopedAStatus::fromServiceSpecificError(CHANNEL_NOT_AVAILABLE);
        }

        // If the AID is defined (the AID is not Null and the length of the
        // AID is not 0) then the corresponding applet shall be selected.
        if (aid.size() > 0) {
            if (isPrefix(aid, kAndroidTestAid)) {
                selectResponse = kAndroidTestSelectResponse;
            } else {
                // No applet registered with matching AID.
                return ScopedAStatus::fromServiceSpecificError(NO_SUCH_ELEMENT_ERROR);
            }
        }

        // If the length of the AID is 0, the method will select the
        // Issuer Security Domain of the SE by sending a SELECT command
        // with 0 length AID as defined in [GPCS].
        if (aid.size() == 0) {
            selectResponse = kIssuerSecurityDomainSelectResponse;
        }

        LOG(INFO) << __func__ << " sending response: "
                  << HexString(selectResponse.data(), selectResponse.size());

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.
        mLogicalChannels[channelNumber] = Channel(aid, p2);
        *aidlReturn = LogicalChannelResponse{
                .channelNumber = static_cast<int8_t>(channelNumber + 1),
                .selectResponse = selectResponse,
        };
        return ScopedAStatus::ok();
    }

    ScopedAStatus closeChannel(int8_t channelNumber) override {
        LOG(INFO) << __func__ << " channel number: " << channelNumber;
        // The basic channel cannot be closed.
        if (channelNumber == 0) {
            return ScopedAStatus::fromExceptionCode(FAILED);
        }

        // The selected logical channel is not opened.
        if (channelNumber > mLogicalChannels.size() ||
            !mLogicalChannels[channelNumber - 1].opened) {
            return ScopedAStatus::ok();
        }

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.
        mLogicalChannels[channelNumber - 1].opened = false;
        return ScopedAStatus::ok();
    }

    ScopedAStatus transmit(const std::vector<uint8_t>& data,
                           std::vector<uint8_t>* _aidl_return) override {
        LOG(INFO) << __func__ << " data: " << HexString(data.data(), data.size()) << " ("
                  << data.size() << ")";

        // TODO(b/123254068) - this is not an implementation of the OMAPI protocol or APDU.
        // The functionality here is enough to exercise the framework, but actual
        // calls to the secure element will fail. This implementation does not model
        // channel isolation or any other aspects important to implementing secure element.

        std::string hex = HexString(data.data(), data.size());                    // DO NOT COPY
        if (hex == "01a4040210a000000476416e64726f696443545331") {                // DO NOT COPY
            *_aidl_return = {0x00, 0x6A, 0x00};                                   // DO NOT COPY
        } else if (data == std::vector<uint8_t>{0x00, 0xF4, 0x00, 0x00, 0x00}) {  // DO NOT COPY
            // CHECK_SELECT_P2_APDU w/ channel 1 // DO NOT COPY
            *_aidl_return = {0x00, 0x90, 0x00};                                   // DO NOT COPY
        } else if (data == std::vector<uint8_t>{0x01, 0xF4, 0x00, 0x00, 0x00}) {  // DO NOT COPY
            // CHECK_SELECT_P2_APDU w/ channel 1 // DO NOT COPY
            *_aidl_return = {0x00, 0x90, 0x00};             // DO NOT COPY
        } else if (data.size() == 5 || data.size() == 8) {  // DO NOT COPY
            // SEGMENTED_RESP_APDU - happens to use length 5 and 8 // DO NOT COPY
            size_t size = (data[2] << 8 | data[3]) + 2;       // DO NOT COPY
            _aidl_return->resize(size);                       // DO NOT COPY
            (*_aidl_return)[size - 1] = 0x00;                 // DO NOT COPY
            (*_aidl_return)[size - 2] = 0x90;                 // DO NOT COPY
            if (size >= 3) (*_aidl_return)[size - 3] = 0xFF;  // DO NOT COPY
        } else {                                              // DO NOT COPY
            *_aidl_return = {0x90, 0x00, 0x00};               // DO NOT COPY
        }                                                     // DO NOT COPY

        return ScopedAStatus::ok();
    }

  private:
    class Channel {
      public:
        Channel() = default;
        Channel(Channel const&) = default;
        Channel(std::vector<uint8_t> const& aid, uint8_t p2) : opened(true), aid(aid), p2(p2) {}
        Channel& operator=(Channel const&) = default;

        bool opened{false};
        std::vector<uint8_t> aid{};
        uint8_t p2{0};
    };

    // OMAPI abstraction.

    Channel mBasicChannel{};
    std::array<Channel, 19> mLogicalChannels{};
    std::shared_ptr<ISecureElementCallback> mClientCallback;

    // Secure element abstraction.

    // Secure element ATR (Answer-To-Reset).
    // The format is specified by ISO/IEC 1816-4 2020 and lists
    // the capabilities of the card.
    std::vector<uint8_t> const mATR{
            // Category indicator byte
            0x0,

            // List of data objects in COMPACT-TLV format.
            // The COMPACT-TLV format has a Tag in the first nibble of a
            // byte (bit 5-8) and a length in the second nibble (bit 1-4).
            0x73,  // Card capabilities.
            0x00,  // Supported selection methods.
            0x00,  // Data coding.
            0x3f,  // No command chaining, no extended Lc / Le fields,
                   // maximum number of logical channels

            // Status indicator byte:
            // No information given; card does not offer life cycle management,
            // commands TERMINATE DF and ACTIVATE FILE are not supported.
            0x0,

            // Processing status bytes SW1/SW2.
            0x90, 0x00};
};

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);

    auto se = ndk::SharedRefBase::make<EmulatedSecureElement>();
    const std::string name = std::string() + BnSecureElement::descriptor + "/eSE1";
    binder_status_t status = AServiceManager_addService(se->asBinder().get(), name.c_str());
    CHECK_EQ(status, STATUS_OK);

    ABinderProcess_joinThreadPool();
    return EXIT_FAILURE;  // should not reach
}
