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

#include "core-impl/DevicePortProxy.h"
#include "core-impl/Module.h"
#include "core-impl/Stream.h"

namespace aidl::android::hardware::audio::core {

class DriverBluetooth : public DriverInterface {
  public:
    DriverBluetooth(const StreamContext& context, bool isInput);
    ::android::status_t init() override;
    ::android::status_t setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& connectedDevices)
            override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t standby() override;
    ::android::status_t prepareToClose() override;
    ::android::status_t close() override;
    ::android::status_t signalBluetoothParameters(
            const std::weak_ptr<BnBluetooth> bluetooth,
            const std::weak_ptr<BnBluetoothA2dp> bluetoothA2dp,
            const std::weak_ptr<BnBluetoothLe> bluetoothLe) override;

    bool updateMetadata(const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata);
    bool updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata);

  private:
    ::android::status_t doInit();
    bool checkConfigParams(::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config);

    // Audio Pcm Config
    uint32_t mSampleRate;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    size_t mFrameSizeBytes;
    const bool mIsInput;

    size_t mPreferredDataIntervalUs;
    size_t mPreferredFrameCount;

    bool mInitDone GUARDED_BY(mListLock);
    bool mIsReadyToClose GUARDED_BY(mStreamLock);

    // lock to manage vector of bluetooth port proxy handles. For each connected device, a proxy
    // handle is created. On successful instantiation this is pushed to mBtDeviceProxies list. Reads
    // and writes to this list is guarded by mListLock.
    std::mutex mListLock;
    // Once the proxy list is created, interacting with underlying bluetooth module is achieved via
    // proxy api calls. Calls to these are made only after acquiring mStreamLock.
    std::mutex mStreamLock;

    // Cached device addresses for connected devices.
    std::vector<::aidl::android::media::audio::common::AudioDevice> mConnectedDevices
            GUARDED_BY(mListLock);
    std::vector<std::shared_ptr<::android::bluetooth::audio::aidl::BluetoothAudioPort>>
            mBtDeviceProxies GUARDED_BY(mListLock);
};

class StreamInBluetooth final : public StreamIn {
  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result);

    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SinkMetadata& in_sinkMetadata) override;

  private:
    friend class ndk::SharedRefBase;
    StreamInBluetooth(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);
};

class StreamOutBluetooth final : public StreamOut {
  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result);

    ndk::ScopedAStatus updateMetadata(
            const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata)
            override;

  private:
    friend class ndk::SharedRefBase;
    StreamOutBluetooth(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo);
};

}  // namespace aidl::android::hardware::audio::core
