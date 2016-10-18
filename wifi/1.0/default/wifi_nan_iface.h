/*
 * Copyright (C) 2016 The Android Open Source Project
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

#ifndef WIFI_NAN_IFACE_H_
#define WIFI_NAN_IFACE_H_

#include <android-base/macros.h>
#include <android/hardware/wifi/1.0/IWifiNanIface.h>
#include <android/hardware/wifi/1.0/IWifiNanIfaceEventCallback.h>

#include "wifi_legacy_hal.h"

namespace android {
namespace hardware {
namespace wifi {
namespace V1_0 {
namespace implementation {

/**
 * HIDL interface object used to control a NAN Iface instance.
 */
class WifiNanIface : public IWifiNanIface {
 public:
  WifiNanIface(const std::string& ifname,
               const std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal);
  // Refer to |WifiChip::invalidate()|.
  void invalidate();

  // HIDL methods exposed.
  Return<void> getName(getName_cb hidl_status_cb) override;
  Return<void> getType(getType_cb hidl_status_cb) override;
  Return<void> registerEventCallback(
      const sp<IWifiNanIfaceEventCallback>& event_callback,
      registerEventCallback_cb hidl_status_cb) override;
  Return<void> enableRequest(uint32_t cmdId,
                             const NanEnableRequest& msg,
                             enableRequest_cb hidl_status_cb) override;
  Return<void> disableRequest(uint32_t cmdId,
                              disableRequest_cb hidl_status_cb) override;
  Return<void> publishRequest(uint32_t cmdId,
                              const NanPublishRequest& msg,
                              publishRequest_cb hidl_status_cb) override;
  Return<void> publishCancelRequest(
      uint32_t cmdId,
      const NanPublishCancelRequest& msg,
      publishCancelRequest_cb hidl_status_cb) override;
  Return<void> subscribeRequest(uint32_t cmdId,
                                const NanSubscribeRequest& msg,
                                subscribeRequest_cb hidl_status_cb) override;
  Return<void> subscribeCancelRequest(
      uint32_t cmdId,
      const NanSubscribeCancelRequest& msg,
      subscribeCancelRequest_cb hidl_status_cb) override;
  Return<void> transmitFollowupRequest(
      uint32_t cmdId,
      const NanTransmitFollowupRequest& msg,
      transmitFollowupRequest_cb hidl_status_cb) override;
  Return<void> configRequest(uint32_t cmdId,
                             const NanConfigRequest& msg,
                             configRequest_cb hidl_status_cb) override;
  Return<void> beaconSdfPayloadRequest(
      uint32_t cmdId,
      const NanBeaconSdfPayloadRequest& msg,
      beaconSdfPayloadRequest_cb hidl_status_cb) override;
  Return<void> getVersion(getVersion_cb hidl_status_cb) override;
  Return<void> getCapabilities(uint32_t cmdId,
                               getCapabilities_cb hidl_status_cb) override;
  Return<void> dataInterfaceCreate(
      uint32_t cmdId,
      const hidl_string& ifaceName,
      dataInterfaceCreate_cb hidl_status_cb) override;
  Return<void> dataInterfaceDelete(
      uint32_t cmdId,
      const hidl_string& ifaceName,
      dataInterfaceDelete_cb hidl_status_cb) override;
  Return<void> dataRequestInitiator(
      uint32_t cmdId,
      const NanDataPathInitiatorRequest& msg,
      dataRequestInitiator_cb hidl_status_cb) override;
  Return<void> dataIndicationResponse(
      uint32_t cmdId,
      const NanDataPathIndicationResponse& msg,
      dataIndicationResponse_cb hidl_status_cb) override;
  Return<void> dataEnd(uint32_t cmdId,
                       const NanDataPathEndRequest& msg,
                       dataEnd_cb hidl_status_cb) override;

 private:
  std::string ifname_;
  std::weak_ptr<legacy_hal::WifiLegacyHal> legacy_hal_;
  std::vector<sp<IWifiNanIfaceEventCallback>> event_callbacks_;
  bool is_valid_;

  DISALLOW_COPY_AND_ASSIGN(WifiNanIface);
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace wifi
}  // namespace hardware
}  // namespace android

#endif  // WIFI_NAN_IFACE_H_
