/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include <mutex>
#include <vector>

#include <aidl/android/hardware/audio/core/IBluetooth.h>
#include <aidl/android/hardware/audio/core/IBluetoothA2dp.h>
#include <aidl/android/hardware/audio/core/IBluetoothLe.h>

#include "core-impl/DevicePortProxy.h"
#include "core-impl/Module.h"
#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

class StreamBluetooth : public StreamCommonImpl {
  public:
    StreamBluetooth(const Metadata& metadata, StreamContext&& context,
                    Module::BtProfileHandles&& btHandles);
    // Methods of 'DriverInterface'.
    ::android::status_t init() override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t standby() override;
    void shutdown() override;

    // Overridden methods of 'StreamCommonImpl', called on a Binder thread.
    ndk::ScopedAStatus prepareToClose() override;
    const ConnectedDevices& getConnectedDevices() const override;
    ndk::ScopedAStatus setConnectedDevices(const ConnectedDevices& devices) override;
    ndk::ScopedAStatus onBluetoothParametersUpdated() override;

  private:
    // Audio Pcm Config
    uint32_t mSampleRate;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    size_t mFrameSizeBytes;
    const bool mIsInput;
    const std::weak_ptr<IBluetooth> mBluetooth;
    const std::weak_ptr<IBluetoothA2dp> mBluetoothA2dp;
    const std::weak_ptr<IBluetoothLe> mBluetoothLe;
    size_t mPreferredDataIntervalUs;
    size_t mPreferredFrameCount;

    mutable std::mutex mLock;
    bool mInitDone GUARDED_BY(mLock);
    bool mIsReadyToClose GUARDED_BY(mLock);
    std::vector<std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPortAidl>>
            mBtDeviceProxies GUARDED_BY(mLock);

    ::android::status_t doInit() REQUIRES(mLock);
    bool checkConfigParams(::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config);

  protected:
    bool updateSinkMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata);
    bool updateSourceMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata);
};

class StreamInBluetooth final : public StreamBluetooth, public StreamIn {
  public:
    friend class ndk::SharedRefBase;
    StreamInBluetooth(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            Module::BtProfileHandles&& btHandles);

  private:
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata& in_sinkMetadata) override;
};

class StreamOutBluetooth final : public StreamBluetooth, public StreamOut {
  public:
    friend class ndk::SharedRefBase;
    StreamOutBluetooth(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            Module::BtProfileHandles&& btHandles);

  private:
    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override;
};

}  // namespace aidl::android::hardware::audio::core
