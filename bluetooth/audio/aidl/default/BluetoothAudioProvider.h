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
#pragma once

#include <BluetoothAudioSessionReport.h>
#include <aidl/android/hardware/bluetooth/audio/BnBluetoothAudioProvider.h>
#include <aidl/android/hardware/bluetooth/audio/LatencyMode.h>
#include <aidl/android/hardware/bluetooth/audio/SessionType.h>
#include <android-base/logging.h>
#include <fmq/AidlMessageQueue.h>

using ::aidl::android::hardware::common::fmq::MQDescriptor;
using ::aidl::android::hardware::common::fmq::SynchronizedReadWrite;
using ::android::AidlMessageQueue;

using MqDataType = int8_t;
using MqDataMode = SynchronizedReadWrite;
using DataMQ = AidlMessageQueue<MqDataType, MqDataMode>;
using DataMQDesc = MQDescriptor<MqDataType, MqDataMode>;

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

class BluetoothAudioProviderContext {
 private:
  std::mutex _mutex;
  SessionType _session_type;
  std::shared_ptr<IBluetoothAudioPort> _stack_iface;
  std::unique_ptr<const AudioConfiguration> _audio_config;

 public:
  BluetoothAudioProviderContext(
      SessionType session_type,
      const std::shared_ptr<IBluetoothAudioPort>& host_if,
      const AudioConfiguration& audio_config)
      : _session_type(session_type),
        _stack_iface(host_if),
        _audio_config(std::make_unique<AudioConfiguration>(audio_config)) {}

  ~BluetoothAudioProviderContext() {
    std::unique_lock<std::mutex> lock(_mutex);
  }

  static void start(BluetoothAudioProviderContext* cookie,
                    ::ndk::ScopedAIBinder_DeathRecipient& recipient) {
    if (!cookie) {
      LOG(ERROR) << __func__ << " invalid null cookie";
      return;
    }
    std::unique_lock<std::mutex> lock(cookie->_mutex);

    if (!cookie->_stack_iface) {
      LOG(ERROR) << __func__ << " invalid null stack iface";
      return;
    }
    AIBinder_linkToDeath(cookie->_stack_iface->asBinder().get(),
                         recipient.get(), cookie);
  }

  static void end(BluetoothAudioProviderContext* cookie,
                  ::ndk::ScopedAIBinder_DeathRecipient* recipient) {
    if (!cookie) {
      LOG(ERROR) << __func__ << " invalid null cookie";
      return;
    }
    std::unique_lock<std::mutex> lock(cookie->_mutex);

    // if _stack_iface is null, that mean we already ended the session
    if (!cookie->_stack_iface) {
      LOG(ERROR) << __func__
                 << " - SessionType=" << toString(cookie->_session_type)
                 << " invalid null stack iface: has NO session";
      return;
    }

    BluetoothAudioSessionReport::OnSessionEnded(cookie->_session_type);

    if (recipient) {
      AIBinder_unlinkToDeath(cookie->_stack_iface->asBinder().get(),
                             recipient->get(), cookie);
    }

    cookie->_stack_iface = nullptr;
    cookie->_audio_config = nullptr;

    LOG(INFO) << __func__
              << " - SessionType=" << toString(cookie->_session_type);
  }

  static bool is_valid_session(BluetoothAudioProviderContext* cookie) {
    return cookie && cookie->_stack_iface;
  }

  static bool updateAudioConfiguration(BluetoothAudioProviderContext* cookie,
                                       const AudioConfiguration& audio_config) {
    if (!cookie) {
      LOG(ERROR) << __func__
                 << " invalid null cookie. audio=" << audio_config.toString();
      return false;
    }
    std::unique_lock<std::mutex> lock(cookie->_mutex);

    // if _stack_iface  is null, that mean we already ended the session
    if (!cookie->_stack_iface) {
      LOG(ERROR) << __func__
                 << " - SessionType=" << toString(cookie->_session_type)
                 << " invalid null stack iface: has NO session";
      return false;
    }

    if (audio_config.getTag() != cookie->_audio_config->getTag()) {
      LOG(ERROR) << __func__
                 << " - SessionType=" << toString(cookie->_session_type)
                 << " audio config type does not match";
      return false;
    }

    cookie->_audio_config =
        std::make_unique<const AudioConfiguration>(audio_config);
    return true;
  }

  static std::shared_ptr<IBluetoothAudioPort> getStackIface(
      BluetoothAudioProviderContext* cookie) {
    // Unchecked deref.
    // This is called from OnSessionReady and should always be valid
    return cookie->_stack_iface;
  }

  static const AudioConfiguration& getAudioConfig(
      BluetoothAudioProviderContext* cookie) {
    // Unchecked deref.
    // This is called from OnSessionReady and should always be valid
    return *cookie->_audio_config;
  }
};

class BluetoothAudioProvider : public BnBluetoothAudioProvider {
 public:
  BluetoothAudioProvider();
  ndk::ScopedAStatus startSession(
      const std::shared_ptr<IBluetoothAudioPort>& host_if,
      const AudioConfiguration& audio_config,
      const std::vector<LatencyMode>& latency_modes,
      DataMQDesc* _aidl_return);
  ndk::ScopedAStatus endSession();
  ndk::ScopedAStatus streamStarted(BluetoothAudioStatus status);
  ndk::ScopedAStatus streamSuspended(BluetoothAudioStatus status);
  ndk::ScopedAStatus updateAudioConfiguration(
      const AudioConfiguration& audio_config);
  ndk::ScopedAStatus setLowLatencyModeAllowed(bool allowed);

  virtual bool isValid(const SessionType& sessionType) = 0;

 protected:
  virtual ndk::ScopedAStatus onSessionReady(DataMQDesc* _aidl_return) = 0;

  ::ndk::ScopedAIBinder_DeathRecipient death_recipient_;

  BluetoothAudioProviderContext* cookie_context;
  std::shared_ptr<IBluetoothAudioPort> getStackIface();
  const AudioConfiguration& getAudioConfig();

  SessionType session_type_;
  std::vector<LatencyMode> latency_modes_;
};
}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl
