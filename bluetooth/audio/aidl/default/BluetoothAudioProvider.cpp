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

#define LOG_TAG "BTAudioProviderStub"

#include "BluetoothAudioProvider.h"

#include <BluetoothAudioSessionReport.h>
#include <android-base/logging.h>

namespace aidl {
namespace android {
namespace hardware {
namespace bluetooth {
namespace audio {

BluetoothAudioProvider::BluetoothAudioProvider() {
  death_recipient_ = ::ndk::ScopedAIBinder_DeathRecipient(
      AIBinder_DeathRecipient_new(onBinderDied));
  AIBinder_DeathRecipient_setOnUnlinked(death_recipient_.get(),
                                        onBinderUnlinked);
}

ndk::ScopedAStatus BluetoothAudioProvider::startSession(
    const std::shared_ptr<IBluetoothAudioPort>& host_if,
    const AudioConfiguration& audio_config,
    const std::vector<LatencyMode>& latencyModes,
    DataMQDesc* _aidl_return) {
  if (host_if == nullptr) {
    *_aidl_return = DataMQDesc();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  if (mOnBinderDiedContext != nullptr) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
  }

  latency_modes_ = latencyModes;
  audio_config_ = std::make_unique<AudioConfiguration>(audio_config);
  stack_iface_ = host_if;
  is_binder_died = false;

  mOnBinderDiedContext =
      std::make_unique<OnBinderDiedContext>(OnBinderDiedContext{
          .btAudioProvider = this->ref<BluetoothAudioProvider>()});
  // We know context must be alive when we use contextPtr because context would
  // only be removed in OnBinderUnlinked, which must be called after
  // OnBinderDied.
  void* contextPtr = static_cast<void*>(mOnBinderDiedContext.get());
  // Insert into a map to keep the context object alive.

  AIBinder_linkToDeath(stack_iface_->asBinder().get(), death_recipient_.get(),
                       contextPtr);

  onSessionReady(_aidl_return);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothAudioProvider::endSession() {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);

  if (stack_iface_ != nullptr) {
    BluetoothAudioSessionReport::OnSessionEnded(session_type_);

    if (!is_binder_died) {
      AIBinder_unlinkToDeath(stack_iface_->asBinder().get(),
                             death_recipient_.get(), this);
    }
  } else {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
  }

  stack_iface_ = nullptr;
  audio_config_ = nullptr;

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothAudioProvider::streamStarted(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  if (stack_iface_ != nullptr) {
    BluetoothAudioSessionReport::ReportControlStatus(session_type_, true,
                                                     status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }

  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothAudioProvider::streamSuspended(
    BluetoothAudioStatus status) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
            << ", status=" << toString(status);

  if (stack_iface_ != nullptr) {
    BluetoothAudioSessionReport::ReportControlStatus(session_type_, false,
                                                     status);
  } else {
    LOG(WARNING) << __func__ << " - SessionType=" << toString(session_type_)
                 << ", status=" << toString(status) << " has NO session";
  }
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothAudioProvider::updateAudioConfiguration(
    const AudioConfiguration& audio_config) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);

  if (stack_iface_ == nullptr || audio_config_ == nullptr) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  if (audio_config.getTag() != audio_config_->getTag()) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " audio config type is not match";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }

  audio_config_ = std::make_unique<AudioConfiguration>(audio_config);
  BluetoothAudioSessionReport::ReportAudioConfigChanged(session_type_,
                                                        *audio_config_);
  return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus BluetoothAudioProvider::setLowLatencyModeAllowed(
    bool allowed) {
  LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_);

  if (stack_iface_ == nullptr) {
    LOG(INFO) << __func__ << " - SessionType=" << toString(session_type_)
              << " has NO session";
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
  }
  LOG(INFO) << __func__ << " - allowed " << allowed;
  BluetoothAudioSessionReport::ReportLowLatencyModeAllowedChanged(
    session_type_, allowed);
  return ndk::ScopedAStatus::ok();
}

void BluetoothAudioProvider::onBinderDied(void* cookie) {
  OnBinderDiedContext* context = reinterpret_cast<OnBinderDiedContext*>(cookie);
  std::shared_ptr<BluetoothAudioProvider> provider =
      context->btAudioProvider.lock();
  if (!provider) {
    LOG(ERROR) << __func__ << ": AudioProvider HAL died";
    return;
  }
  LOG(DEBUG) << __func__ << ": BluetoothAudio binder died";
  provider->is_binder_died = true;
  provider->endSession();
}

void BluetoothAudioProvider::onBinderUnlinked(void* cookie) {
  LOG(DEBUG) << __func__ << ": BluetoothAudio binder unlinked";
  OnBinderDiedContext* context = reinterpret_cast<OnBinderDiedContext*>(cookie);
  if (context->btAudioProvider.expired()) {
    LOG(ERROR) << __func__ << ": AudioProvider HAL died";
  }
  // Delete the context associated with this cookie.
  context->btAudioProvider.reset();
}

}  // namespace audio
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
}  // namespace aidl