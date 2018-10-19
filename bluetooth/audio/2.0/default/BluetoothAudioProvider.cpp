/*
 * Copyright (C) 2018 The Android Open Source Project
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

#define LOG_TAG "BTAudioProviderStub"

#include <log/log.h>

#include "BluetoothAudioProvider.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::hidl_death_recipient;
using android::hardware::Void;

BluetoothAudioProvider::BluetoothAudioProvider()
    : has_session_(false), sessionType(SessionType::UNKNOWN),
      ctrl_res_cb_(nullptr), session_ended_cb_(nullptr),
      death_recipient_(new BluetoothAudioDeathRecipient(this)),
      unlink_cb_(nullptr) {

  // FIXME: which number of frameSize and framesCount should we use?
  std::unique_ptr<DataMQ> tempDataMQ(new DataMQ(/* frameSize*framesCount */ 4*128, /* EventFlag */ true));
  if (!tempDataMQ->isValid()) {
    ALOGE_IF(!tempDataMQ->isValid(), "data MQ is invalid");
  } else {
    mDataMQ = std::move(tempDataMQ);
  }
}

// TODO
//BluetoothAudioProvider::~BluetoothAudioProvider() {
//}

Return<void> BluetoothAudioProvider::startSession(
    const sp<IBluetoothAudioPort>& hostIf,
    const SessionType sessionType,
    const CodecConfiguration& codecConfig, startSession_cb _hidl_cb) {
  BluetoothAudioStatus result;

  result = BluetoothAudioStatus::SUCCESS;
  ALOGI("%s - SessionType=0x%02hhx, CodecConfig={Codec=0x%08x, MTU=0x%04x}", __func__,
         sessionType, codecConfig.encodedDataConfiguration.codecType, codecConfig.encodedDataConfiguration.peerMtu);
  // TODO: should we check whether the codec is supported or not?

  death_recipient_->setHasDied(false);
  hostIf->linkToDeath(death_recipient_, 0);

  /**
   * Initialize the audio platform if codecConfiguration is supported.
   * Save the the IBluetoothAudioPort interface, so that it can be used
   * later to send stream control commands to the HAL client, based on
   * interaction with Audio framework.
   */

  has_session_ = true;
  this->sessionType = sessionType;
  stack_iface_ = hostIf;

  unlink_cb_ = [hostIf](sp<BluetoothAudioDeathRecipient>& death_recipient) {
    if (death_recipient->getHasDied())
      ALOGI("Skipping unlink call, service died.");
    else
      hostIf->unlinkToDeath(death_recipient);
  };

  _hidl_cb(result, *mDataMQ->getDesc());
  return Void();
}

Return<void> BluetoothAudioProvider::streamStarted(BluetoothAudioStatus status) {

  ALOGI("%s - status=0x%02hhx", __func__, status);

  /**
   * Streaming on control path has started,
   * HAL server should start the streaming on data path.
   */
  if (ctrl_res_cb_)
    //ctrl_res_cb_(SessionType::UNKNOWN, status);
    ctrl_res_cb_(sessionType, status);
  else
    ALOGW("%s - status=0x%02hhx no monitor", __func__, status);

  return Void();
}

Return<void> BluetoothAudioProvider::streamSuspended(BluetoothAudioStatus status) {

  ALOGI("%s - status=0x%02hhx", __func__, status);

  /**
   * Streaming on control path has suspend,
   * HAL server should suspend the streaming on data path.
   */
  if (ctrl_res_cb_)
    //ctrl_res_cb_(SessionType::UNKNOWN, status);
    ctrl_res_cb_(sessionType, status);
  else
    ALOGW("%s - status=0x%02hhx no monitor", __func__, status);

  return Void();
}

Return<void> BluetoothAudioProvider::endSession() {

  ALOGI("%s", __func__);

  if (session_ended_cb_)
    //session_ended_cb_(SessionType::UNKNOWN);
    session_ended_cb_(sessionType);
  else
    ALOGW("%s - no monitor", __func__);

  /**
   * Cleanup the audio platform as remote audio device is no
   * longer active
   */
  has_session_ = false;
  sessionType = SessionType::UNKNOWN;
  stack_iface_ = nullptr;
  ctrl_res_cb_ = nullptr;
  session_ended_cb_ = nullptr;

  unlink_cb_(death_recipient_);
  death_recipient_->setHasDied(true);
  unlink_cb_ = nullptr;

  return Void();
}

hidl_vec<uint8_t>* BluetoothAudioProvider::getStreamDataBuffer() {
  if (has_session_)
    return &mStreamBuffer;
  return nullptr;
}

const DataMQ::Descriptor* BluetoothAudioProvider::getStreamDataFMQ() {
  if (has_session_ && mDataMQ->isValid())
    return mDataMQ->getDesc();
  return nullptr;
}

const sp<IBluetoothAudioPort> BluetoothAudioProvider::getAssociatedPortCtrl() {
  if (has_session_)
    return stack_iface_;
  return nullptr;
}

Return<void> BluetoothAudioProvider::registerControlResultCback(std::function<void(SessionType&, BluetoothAudioStatus&)>& ctrl_res_cb,
                                                                std::function<void(SessionType&)>& session_ended_cb) {
  if (ctrl_res_cb)
    ctrl_res_cb_ = ctrl_res_cb;
  if (session_ended_cb)
    session_ended_cb_ = session_ended_cb;
  return Void();
}

Return<void> BluetoothAudioProvider::unregisterControlResultCback() {
  ctrl_res_cb_ = nullptr;
  session_ended_cb_ = nullptr;
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
