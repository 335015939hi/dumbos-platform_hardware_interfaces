/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

#include <android/hardware/wifi/1.0/IWifiNanIfaceEventCallback.h>

namespace {
namespace nan {

using namespace ::android::hardware::wifi::V1_0;

using ::android::hardware::Return;
using ::android::hardware::Void;

/*
 * Types of all notification and event callbacks. Can be used as a bit mask.
 */
enum CallbackType {
  TIMEOUT = -1,

  NOTIFY_CAPABILITIES_RESPONSE = 0x1 << 0,
  NOTIFY_ENABLE_RESPONSE = 0x1 << 1,
  NOTIFY_CONFIG_RESPONSE = 0x1 << 2,
  NOTIFY_DISABLE_RESPONSE = 0x1 << 3,
  NOTIFY_START_PUBLISH_RESPONSE = 0x1 << 3,
  NOTIFY_STOP_PUBLISH_RESPONSE = 0x1 << 4,
  NOTIFY_START_SUBSCRIBE_RESPONSE = 0x1 << 5,
  NOTIFY_STOP_SUBSCRIBE_RESPONSE = 0x1 << 6,
  NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE = 0x1 << 7,
  NOTIFY_CREATE_DATA_INTERFACE_RESPONSE = 0x1 << 8,
  NOTIFY_DELETE_DATA_INTERFACE_RESPONSE = 0x1 << 9,
  NOTIFY_INITIATE_DATA_PATH_RESPONSE = 0x1 << 10,
  NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE = 0x1 << 11,
  NOTIFY_TERMINATE_DATA_PATH_RESPONSE = 0x1 << 12,

  EVENT_CLUSTER_EVENT = 0x1 << 13,
  EVENT_DISABLED = 0x1 << 14,
  EVENT_PUBLISH_TERMINATED = 0x1 << 15,
  EVENT_SUBSCRIBE_TERMINATED = 0x1 << 16,
  EVENT_MATCH = 0x1 << 17,
  EVENT_MATCH_EXPIRED = 0x1 << 18,
  EVENT_FOLLOWUP_RECEIVED = 0x1 << 19,
  EVENT_TRANSMIT_FOLLOWUP = 0x1 << 20,
  EVENT_DATA_PATH_REQUEST = 0x1 << 21,
  EVENT_DATA_PATH_CONFIRM = 0x1 << 22,
  EVENT_DATA_PATH_TERMINATED = 0x1 << 23
};

class WifiNanIfaceEventCallback: public IWifiNanIfaceEventCallback {
 public:
  WifiNanIfaceEventCallback() {};

  virtual ~WifiNanIfaceEventCallback() = default;

  /*
   * Data from IWifiNanIfaceEventCallback callbacks: this is the collection of
   * all arguments to all callbacks. They are set by the callback (notifications
   * or events) and can be retrieved by tests.
   */
  class CallbackData {
   public:
    CallbackData(int callbackType) : callbackType(callbackType) {}

    int callbackType;

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
   * Wait (block) on any callback in the callback mask.
   */
  inline std::shared_ptr<CallbackData>
  wait(int waitForCallbackMask, int timeoutSec) {
    std::unique_lock<std::mutex> lock(mtx_);

    EXPECT_NE(TIMEOUT, waitForCallbackMask);

    std::shared_ptr<CallbackData> cbd = getAndRemoveFirst(waitForCallbackMask);
    if (cbd != nullptr) {
      return cbd;
    }

    waitForCallbackMask_ = waitForCallbackMask;
    std::cv_status status = cv_.wait_until(lock,
                                           std::chrono::system_clock::now()
                                               + std::chrono::seconds(timeoutSec));
    waitForCallbackMask_ = 0;
    if (status == std::cv_status::timeout) {
      return std::make_shared<CallbackData>(TIMEOUT);
    }

    return getAndRemoveFirst(waitForCallbackMask);
  }

  /*
   * Callbacks inherited from IWifiNanIfaceEventCallback.
   */

  Return<void> notifyCapabilitiesResponse(
      uint16_t id,
      const WifiNanStatus &status,
      const NanCapabilities &capabilities) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_CAPABILITIES_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    cbd->capabilities = capabilities;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyEnableResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_ENABLE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyConfigResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_CONFIG_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyDisableResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_DISABLE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyStartPublishResponse(
      uint16_t id,
      const WifiNanStatus &status,
      uint8_t sessionId) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_START_PUBLISH_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    cbd->sessionId = sessionId;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyStopPublishResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_STOP_PUBLISH_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyStartSubscribeResponse(
      uint16_t id,
      const WifiNanStatus &status,
      uint8_t sessionId) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_START_SUBSCRIBE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    cbd->sessionId = sessionId;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyStopSubscribeResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_STOP_SUBSCRIBE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyTransmitFollowupResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_TRANSMIT_FOLLOWUP_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyCreateDataInterfaceResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_CREATE_DATA_INTERFACE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyDeleteDataInterfaceResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_DELETE_DATA_INTERFACE_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyInitiateDataPathResponse(
      uint16_t id,
      const WifiNanStatus &status,
      uint32_t ndpInstanceId) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_INITIATE_DATA_PATH_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    cbd->ndpInstanceId = ndpInstanceId;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyRespondToDataPathIndicationResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_RESPOND_TO_DATA_PATH_INDICATION_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> notifyTerminateDataPathResponse(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(NOTIFY_TERMINATE_DATA_PATH_RESPONSE);
    cbd->id = id;
    cbd->status = status;
    processCallback(cbd);
    return Void();
  }

  Return<void> eventClusterEvent(
      const NanClusterEventInd &event) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_CLUSTER_EVENT);
    cbd->nanClusterEventInd = event;
    return Void();
  }

  Return<void> eventDisabled(
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_DISABLED);
    cbd->status = status;
    return Void();
  }

  Return<void> eventPublishTerminated(
      uint8_t sessionId,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_PUBLISH_TERMINATED);
    cbd->sessionId = sessionId;
    cbd->status = status;
    return Void();
  }

  Return<void> eventSubscribeTerminated(
      uint8_t sessionId,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_SUBSCRIBE_TERMINATED);
    cbd->sessionId = sessionId;
    cbd->status = status;
    return Void();
  }

  Return<void> eventMatch(
      const NanMatchInd &event) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_MATCH);
    cbd->nanMatchInd = event;
    return Void();
  }

  Return<void> eventMatchExpired(
      uint8_t discoverySessionId,
      uint32_t peerId) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_MATCH_EXPIRED);
    cbd->sessionId = discoverySessionId;
    cbd->peerId = peerId;
    return Void();
  }

  Return<void> eventFollowupReceived(
      const NanFollowupReceivedInd &event) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_FOLLOWUP_RECEIVED);
    cbd->nanFollowupReceivedInd = event;
    return Void();
  }

  Return<void> eventTransmitFollowup(
      uint16_t id,
      const WifiNanStatus &status) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_TRANSMIT_FOLLOWUP);
    cbd->id = id;
    cbd->status = status;
    return Void();
  }

  Return<void> eventDataPathRequest(
      const NanDataPathRequestInd &event) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_DATA_PATH_REQUEST);
    cbd->nanDataPathRequestInd = event;
    return Void();
  }

  Return<void> eventDataPathConfirm(
      const NanDataPathConfirmInd &event) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_DATA_PATH_CONFIRM);
    cbd->nanDataPathConfirmInd = event;
    return Void();
  }

  Return<void> eventDataPathTerminated(
      uint32_t ndpInstanceId) override {
    auto cbd = std::make_shared<CallbackData>(EVENT_DATA_PATH_TERMINATED);
    cbd->ndpInstanceId = ndpInstanceId;
    return Void();
  }


 private:
  std::mutex mtx_;

  int waitForCallbackMask_ = 0;
  std::deque<std::shared_ptr < CallbackData>> callbackQueue_;

  std::condition_variable cv_;
  int count_;

  /*
   * Add the callback data to the queue and possibly exit a blocking wait if
   * the callback matches one of the callbacks being waited for.
   */
  void processCallback(std::shared_ptr <CallbackData> callbackData) {
    std::unique_lock<std::mutex> lock(mtx_);
    callbackQueue_.push_back(callbackData);
    if ((waitForCallbackMask_ & callbackData->callbackType)
        == callbackData->callbackType) {
      notify();
    }
  }

  /*
   * Find, remove, and return the callback data corresponding to first event
   * matching any of the callbacks in the callback mask.
   */
  std::shared_ptr <CallbackData> getAndRemoveFirst(int callbackMask) {
    for (std::deque<std::shared_ptr < CallbackData>>::iterator
        it = callbackQueue_.begin(); it != callbackQueue_.end(); ++it) {
      if ((callbackMask & (*it)->callbackType) == (*it)->callbackType) {
        callbackQueue_.erase(it);
        return *it;
      }
    }

    return nullptr;
  }

  /*
   * Trigger an exit from a wait condition.
   */
  inline void notify() {
    count_++;
    cv_.notify_one();
  }
};

} // namespace nan
} // namespace