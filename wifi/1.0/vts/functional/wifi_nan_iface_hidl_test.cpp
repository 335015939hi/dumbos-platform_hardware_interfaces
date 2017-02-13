/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Nanache License, Version 2.0 (the "License");
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

#include <android/hardware/wifi/1.0/IWifiNanIface.h>
#include <android/hardware/wifi/1.0/IWifiNanIfaceEventCallback.h>

#include <gtest/gtest.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "wifi_hidl_call_util.h"
#include "wifi_hidl_test_utils.h"

using ::android::hardware::wifi::V1_0::NanDataPathConfirmInd;
using ::android::hardware::wifi::V1_0::NanDataPathRequestInd;
using ::android::hardware::wifi::V1_0::NanFollowupReceivedInd;
using ::android::hardware::wifi::V1_0::NanCapabilities;
using ::android::hardware::wifi::V1_0::NanClusterEventInd;
using ::android::hardware::wifi::V1_0::NanMatchInd;
using ::android::hardware::wifi::V1_0::IWifiNanIface;
using ::android::hardware::wifi::V1_0::IWifiNanIfaceEventCallback;
using ::android::hardware::wifi::V1_0::WifiNanStatus;
using ::android::hardware::wifi::V1_0::WifiStatus;
using ::android::hardware::wifi::V1_0::WifiStatusCode;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

#define TIMEOUT_PERIOD 10

/**
 * Fixture to use for all NAN Iface HIDL interface tests.
 */
class WifiNanIfaceHidlTest : public ::testing::Test {
  public:
    virtual void SetUp() override {
      iwifiNanIface = getWifiNanIface();
      ASSERT_NE(nullptr, iwifiNanIface.get());
      ASSERT_EQ(WifiStatusCode::SUCCESS, HIDL_INVOKE(iwifiNanIface, registerEventCallback,
            new WifiNanIfaceEventCallback(*this)).code);
    }

    virtual void TearDown() override {
      stopWifi();
    }

    /* Used as a mechanism to inform the test about data/event callback */
    inline void notify() {
      std::unique_lock<std::mutex> lock(mtx_);
      count_++;
      cv_.notify_one();
    }

    /* Test code calls this function to wait for data/event callback */
    inline std::cv_status wait() {
      std::unique_lock<std::mutex> lock(mtx_);

      std::cv_status status = std::cv_status::no_timeout;
      auto now = std::chrono::system_clock::now();
      while (count_ == 0) {
        status = cv_.wait_until(lock, now + std::chrono::seconds(TIMEOUT_PERIOD));
        if (status == std::cv_status::timeout) return status;
      }
      count_--;
      return status;
    }

    enum CallbackType {
        NOTIFY_CAPABILITIES_RESPONSE,
        NOTIFY_ENABLE_RESPONSE,
        NOTIFY_CONFIG_RESPONSE,
        NOTIFY_DISABLE_RESPONSE,
        NOTIFY_START_PUBLISH_RESPONSE,
        NOTIFY_STOP_PUBLISH_RESPONSE,
        NOTIFY_START_SUBSCRIBE_RESPONSE,
        NOTIFY_STOP_SUBSCRIBE_RESPONSE,
        NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE,
        NOTIFY_CREATE_DATA_INTERFACE_RESPONSE,
        NOTIFY_DELETE_DATA_INTERFACE_RESPONSE,
        NOTIFY_INITIATE_DATA_PATH_RESPONSE,
        NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE,
        NOTIFY_TERMINATE_DATA_PATH_RESPONSE,

        EVENT_CLUSTER_EVENT,
        EVENT_DISABLED,
        EVENT_PUBLISH_TERMINATED,
        EVENT_SUBSCRIBE_TERMINATED,
        EVENT_MATCH,
        EVENT_MATCH_EXPIRED,
        EVENT_FOLLOWUP_RECEIVED,
        EVENT_TRANSMIT_FOLLOWUP,
        EVENT_DATA_PATH_REQUEST,
        EVENT_DATA_PATH_CONFIRM,
        EVENT_DATA_PATH_TERMINATED
    };

    class WifiNanIfaceEventCallback: public IWifiNanIfaceEventCallback {
      WifiNanIfaceHidlTest& parent_;

     public:
      WifiNanIfaceEventCallback(WifiNanIfaceHidlTest& parent) : parent_(parent) {};

      virtual ~WifiNanIfaceEventCallback() = default;

      Return<void> notifyCapabilitiesResponse(
            uint16_t id,
            const WifiNanStatus& status,
            const NanCapabilities& capabilities) override {
        parent_.callbackType = NOTIFY_CAPABILITIES_RESPONSE;

        parent_.id = id;
        parent_.status = status;
        parent_.capabilities = capabilities;

        parent_.notify();
        return Void();
      }

      Return<void> notifyEnableResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_ENABLE_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyConfigResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_CONFIG_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyDisableResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_DISABLE_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyStartPublishResponse(
            uint16_t id,
            const WifiNanStatus& status,
            uint8_t sessionId) override {
        parent_.callbackType = NOTIFY_START_PUBLISH_RESPONSE;

        parent_.id = id;
        parent_.status = status;
        parent_.sessionId = sessionId;

        parent_.notify();
        return Void();
      }

      Return<void> notifyStopPublishResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_STOP_PUBLISH_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyStartSubscribeResponse(
            uint16_t id,
            const WifiNanStatus& status,
            uint8_t sessionId) override {
        parent_.callbackType = NOTIFY_START_SUBSCRIBE_RESPONSE;

        parent_.id = id;
        parent_.status = status;
        parent_.sessionId = sessionId;

        parent_.notify();
        return Void();
      }

      Return<void> notifyStopSubscribeResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_STOP_SUBSCRIBE_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyTransmitFollowupResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyCreateDataInterfaceResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_CREATE_DATA_INTERFACE_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyDeleteDataInterfaceResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_DELETE_DATA_INTERFACE_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyInitiateDataPathResponse(
            uint16_t id,
            const WifiNanStatus& status,
            uint32_t ndpInstanceId) override {
        parent_.callbackType = NOTIFY_INITIATE_DATA_PATH_RESPONSE;

        parent_.id = id;
        parent_.status = status;
        parent_.ndpInstanceId = ndpInstanceId;

        parent_.notify();
        return Void();
      }

      Return<void> notifyRespondToDataPathIndicationResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> notifyTerminateDataPathResponse(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = NOTIFY_TERMINATE_DATA_PATH_RESPONSE;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> eventClusterEvent(
            const NanClusterEventInd& event) override {
        parent_.callbackType = EVENT_CLUSTER_EVENT;

        parent_.nanClusterEventInd = event;

        parent_.notify();
        return Void();
      }

      Return<void> eventDisabled(
            const WifiNanStatus& status) override {
        parent_.callbackType = EVENT_DISABLED;

        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> eventPublishTerminated(
            uint8_t sessionId,
            const WifiNanStatus& status) override {
        parent_.callbackType = EVENT_PUBLISH_TERMINATED;

        parent_.sessionId = sessionId;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> eventSubscribeTerminated(
            uint8_t sessionId,
            const WifiNanStatus& status) override {
        parent_.callbackType = EVENT_SUBSCRIBE_TERMINATED;

        parent_.sessionId = sessionId;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> eventMatch(
            const NanMatchInd& event) override {
        parent_.callbackType = EVENT_MATCH;

        parent_.nanMatchInd = event;

        parent_.notify();
        return Void();
      }

      Return<void> eventMatchExpired(
            uint8_t discoverySessionId,
            uint32_t peerId) override {
        parent_.callbackType = EVENT_MATCH_EXPIRED;

        parent_.sessionId = discoverySessionId;
        parent_.peerId = peerId;

        parent_.notify();
        return Void();
      }

      Return<void> eventFollowupReceived(
            const NanFollowupReceivedInd& event) override {
        parent_.callbackType = EVENT_FOLLOWUP_RECEIVED;

        parent_.nanFollowupReceivedInd = event;

        parent_.notify();
        return Void();
      }

      Return<void> eventTransmitFollowup(
            uint16_t id,
            const WifiNanStatus& status) override {
        parent_.callbackType = EVENT_TRANSMIT_FOLLOWUP;

        parent_.id = id;
        parent_.status = status;

        parent_.notify();
        return Void();
      }

      Return<void> eventDataPathRequest(
            const NanDataPathRequestInd& event) override {
        parent_.callbackType = EVENT_DATA_PATH_REQUEST;

        parent_.nanDataPathRequestInd = event;

        parent_.notify();
        return Void();
      }

      Return<void> eventDataPathConfirm(
            const NanDataPathConfirmInd& event) override {
        parent_.callbackType = EVENT_DATA_PATH_CONFIRM;

        parent_.nanDataPathConfirmInd = event;

        parent_.notify();
        return Void();
      }

      Return<void> eventDataPathTerminated(
            uint32_t ndpInstanceId) override {
        parent_.callbackType = EVENT_DATA_PATH_TERMINATED;

        parent_.ndpInstanceId = ndpInstanceId;

        parent_.notify();
        return Void();
      }
    };

    private:
      // synchronization objects
      std::mutex mtx_;
      std::condition_variable cv_;
      int count_;

    protected:
      android::sp<IWifiNanIface> iwifiNanIface;

      // data from callbacks
      CallbackType callbackType;
      uint16_t id;
      WifiNanStatus status;
      NanCapabilities capabilities;
      uint8_t sessionId;
      uint32_t ndpInstanceId;
      NanClusterEventInd nanClusterEventInd;
      NanMatchInd nanMatchInd;
      uint32_t peerId;
      NanFollowupReceivedInd nanFollowupReceivedInd;
      NanDataPathRequestInd nanDataPathRequestInd;
      NanDataPathConfirmInd nanDataPathConfirmInd;
};

/*
 * Create:
 * Ensures that an instance of the IWifiNanIface proxy object is
 * successfully created.
 */
TEST(WifiNanIfaceHidlTestNoFixture, Create) {
  EXPECT_NE(nullptr, getWifiNanIface().get());
  stopWifi();
}

/*
 * Fail: use past destruction
 * Ensure that API calls fail with ERROR_WIFI_IFACE_INVALID when using an interface once wifi
 * is disabled.
 */
TEST(WifiNanIfaceHidlTestNoFixture, FailOnIfaceInvalid) {
  android::sp<IWifiNanIface> iwifiNanIface = getWifiNanIface();
  EXPECT_NE(nullptr, iwifiNanIface.get());
  stopWifi();
  ASSERT_EQ(WifiStatusCode::ERROR_WIFI_IFACE_INVALID,
          HIDL_INVOKE(iwifiNanIface, getCapabilitiesRequest, 0).code);
}

/*
 * getCapabilitiesRequest: validate that returns capabilities.
 */
TEST_F(WifiNanIfaceHidlTest, getCapabilitiesRequest) {
  uint16_t inputCmdId = 10;
  ASSERT_EQ(WifiStatusCode::SUCCESS,
        HIDL_INVOKE(iwifiNanIface, getCapabilitiesRequest, inputCmdId).code);
  // wait for a callback
  ASSERT_EQ(std::cv_status::no_timeout, wait());
  ASSERT_EQ(NOTIFY_CAPABILITIES_RESPONSE, callbackType);
  ASSERT_EQ(id, inputCmdId);

  // check for reasonable capability values
  EXPECT_GT(capabilities.maxConcurrentClusters, 0);
  EXPECT_GT(capabilities.maxPublishes, 0);
  EXPECT_GT(capabilities.maxSubscribes, 0);
  EXPECT_GT(capabilities.maxServiceNameLen, 0);
  EXPECT_GT(capabilities.maxMatchFilterLen, 0);
  EXPECT_GT(capabilities.maxTotalMatchFilterLen, 0);
  EXPECT_GT(capabilities.maxServiceSpecificInfoLen, 0);
  EXPECT_GT(capabilities.maxExtendedServiceSpecificInfoLen, 0);
  EXPECT_GT(capabilities.maxNdiInterfaces, 0);
  EXPECT_GT(capabilities.maxNdpSessions, 0);
  EXPECT_GT(capabilities.maxAppInfoLen, 0);
  EXPECT_GT(capabilities.maxQueuedTransmitFollowupMsgs, 0);
  EXPECT_GT(capabilities.maxSubscribeInterfaceAddresses, 0);
  EXPECT_NE(capabilities.supportedCipherSuites, 0);
}

