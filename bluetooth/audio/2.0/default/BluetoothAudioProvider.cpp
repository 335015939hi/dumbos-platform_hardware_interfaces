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

#define RTP_FRAME_SIZE   (PCM_FRAME_SIZE * PCM_FRAME_COUNT)
#ifndef RTP_FRAME_COUNT
#define RTP_FRAME_COUNT  14
#endif

#define BUFFER_SIZE      (RTP_FRAME_SIZE * RTP_FRAME_COUNT)
#define DOUBLE_BUFFER    2

namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {
namespace V2_0 {
namespace implementation {

using android::hardware::hidl_death_recipient;
using android::hardware::Void;

BluetoothAudioProvider::BluetoothAudioProvider()
    : has_session_(false),
      ctrl_res_cb_(nullptr), session_ended_cb_(nullptr),
      death_recipient_(new BluetoothAudioDeathRecipient(this)),
      unlink_cb_(nullptr) {

  // FIXME: which number of frameSize and framesCount should we use?
  ALOGI("%s - size of audio data path %d byte(s)", __func__, BUFFER_SIZE*DOUBLE_BUFFER);
  std::unique_ptr<DataMQ> tempDataMQ(new DataMQ(BUFFER_SIZE*DOUBLE_BUFFER, /* EventFlag */ true));
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
    ctrl_res_cb_(SessionType::UNKNOWN, status);
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
    ctrl_res_cb_(SessionType::UNKNOWN, status);
  else
    ALOGW("%s - status=0x%02hhx no monitor", __func__, status);

  return Void();
}

Return<void> BluetoothAudioProvider::endSession() {

  ALOGI("%s", __func__);

  if (session_ended_cb_)
    session_ended_cb_(SessionType::UNKNOWN);
  else
    ALOGW("%s - no monitor", __func__);

  /**
   * Cleanup the audio platform as remote audio device is no
   * longer active
   */
  has_session_ = false;
  stack_iface_ = nullptr;
  ctrl_res_cb_ = nullptr;
  session_ended_cb_ = nullptr;

  unlink_cb_(death_recipient_);
  death_recipient_->setHasDied(true);
  unlink_cb_ = nullptr;

  return Void();
}

const DataMQ::Descriptor* BluetoothAudioProvider::getStreamDataFMQ() {
  if (has_session_) {
    if (mDataMQ != nullptr && mDataMQ->isValid()) {
      return mDataMQ->getDesc();
    } else {
      ALOGW("%s - mDataMQ=%p invalid", __func__, (mDataMQ ? mDataMQ.get() : nullptr));
    }
  } else {
    ALOGW("%s - provider has NO session", __func__);
  }
  return nullptr;
}

const sp<IBluetoothAudioPort> BluetoothAudioProvider::getAssociatedPortCtrl() {
  if (has_session_)
    return stack_iface_;
  return nullptr;
}

int BluetoothAudioProvider::registerControlResultCback(std::function<void(const SessionType, const BluetoothAudioStatus&)>& ctrl_res_cb,
                                                       std::function<void(const SessionType)>& session_ended_cb) {
  // TODO: use std::unordered_map<int, struct cback {}> to store cbacks
  int key = -1;
  if (ctrl_res_cb) {
    if (ctrl_res_cb_)
      ALOGW("%s - ctrl_res_cb overwritten", __func__);
    ctrl_res_cb_ = ctrl_res_cb;
    key = 0;
  }
  if (session_ended_cb) {
    if (session_ended_cb_)
      ALOGW("%s - session_ended_cb overwritten", __func__);
    session_ended_cb_ = session_ended_cb;
    key = 0;
  }
  return key;
}

Return<void> BluetoothAudioProvider::unregisterControlResultCback(const int& key) {
  // TODO: use std::unordered_map<int, struct cback {}> to store cbacks
  if (key > -1) {
    ctrl_res_cb_ = nullptr;
    session_ended_cb_ = nullptr;
  } else {
    ALOGW("%s - invalid control cback", __func__);
  }
  return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
