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

#include <audio_utils/clock.h>
#include <media/nbaio/MonoPipe.h>
#include <media/nbaio/MonoPipeReader.h>

#include <aidl/android/media/audio/common/AudioChannelLayout.h>

#include "core-impl/Stream.h"

using ::android::MonoPipe;
using ::android::MonoPipeReader;
using ::android::sp;

namespace aidl::android::hardware::audio::core {

// Configuration of the submix pipe.
class PipeConfig {
  public:
    int mSampleRate;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    size_t mPipeFrameSize;     // Number of bytes in each audio frame in the pipe.
    size_t mBufferSizeFrames;  // Size of the audio pipe in frames.
    // Maximum number of frames buffered by the input and output streams.
    size_t mBufferPeriodSizeFrames;
};

class SubmixRoute {
  public:
    int id;
    PipeConfig config;
    // Pipe variables: they handle the ring buffer that "pipes" audio:
    //  - from the submix virtual audio output == what needs to be played
    //    remotely, seen as an output for the client
    //  - to the virtual audio source == what is captured by the component
    //    which "records" the submix / virtual audio source, and handles it as needed.
    // A usecase example is one where the component capturing the audio is then sending it over
    // Wifi for presentation on a remote Wifi Display device (e.g. a dongle attached to a TV, or a
    // TV with Wifi Display capabilities), or to a wireless audio player.
    sp<MonoPipe> sink;
    sp<MonoPipeReader> source;

    std::mutex mLock;

    bool mInputOpen = false;
    bool mInputStandby GUARDED_BY(mLock) = true;
    bool mOutputStandbyTransition GUARDED_BY(mLock) = false;
    // how many frames have been requested to be read since standby
    long mReadCounterFrames = 0;
    int mInputRefCount = 0;
    int mReadErrorCount = 0;

    bool mOutputOpen = false;
    bool mOutputStandby GUARDED_BY(mLock) = true;

    // wall clock when recording starts
    std::chrono::time_point<std::chrono::steady_clock> mRecordStartTime;
};

class DriverRemoteSubmix : public DriverInterface {
  public:
    DriverRemoteSubmix(const StreamContext& context, bool isInput);
    ::android::status_t init() override;
    ::android::status_t setConnectedDevices(
            const std::vector<::aidl::android::media::audio::common::AudioDevice>& connectedDevices)
            override;
    ::android::status_t prepareToClose() override;
    ::android::status_t drain(StreamDescriptor::DrainMode) override;
    ::android::status_t flush() override;
    ::android::status_t pause() override;
    ::android::status_t transfer(void* buffer, size_t frameCount, size_t* actualFrameCount,
                                 int32_t* latencyMs) override;
    ::android::status_t standby() override;
    ::android::status_t close() override;

  private:
    bool isValidConfig();
    bool compareConfigs();
    size_t getPipeSizeInFrames();
    ::android::status_t createPipe();
    void releasePipe();
    size_t getStreamPipeSizeInFrames();
    ::android::status_t outWrite(void* buffer, size_t frameCount, size_t* actualFrameCount);
    ::android::status_t inRead(void* buffer, size_t frameCount, size_t* actualFrameCount);

    const bool mIsInput;
    const size_t mFrameSizeBytes;
    ::aidl::android::media::audio::common::AudioFormatDescription mFormat;
    ::aidl::android::media::audio::common::AudioChannelLayout mChannelLayout;
    const int mPortId;
    int mSampleRate;
    ::android::status_t mStatus = ::android::NO_INIT;

    std::mutex mLock;
    std::vector<std::shared_ptr<SubmixRoute>> mSubmixRoutes GUARDED_BY(mLock);
    // Cached device addresses for connected devices.
    std::vector<::aidl::android::media::audio::common::AudioDeviceAddress> mConnectedDevices
            GUARDED_BY(mLock);
    std::shared_ptr<SubmixRoute> mSubmixRoute;

    static constexpr int DEFAULT_SAMPLE_RATE_HZ = 48000;
    // Size at default sample rate
    // NOTE: This value will be rounded up to the nearest power of 2 by MonoPipe().
    static constexpr int DEFAULT_PIPE_SIZE_IN_FRAMES = (1024 * 4);
    // Value used to divide the MonoPipe() buffer into segments that are written to the source and
    // read from the sink.  The maximum latency of the device is the size of the MonoPipe's buffer
    // the minimum latency is the MonoPipe buffer size divided by this value.
    static constexpr int DEFAULT_PIPE_PERIOD_COUNT = 4;
    // limit for number of read error log entries to avoid spamming the logs
    static constexpr int MAX_READ_ERROR_LOGS = 5;
    // The duration of MAX_READ_ATTEMPTS * READ_ATTEMPT_SLEEP_MS must be strictly inferior to
    //   the duration of a record buffer at the current record sample rate (of the device, not of
    //   the recording itself). Here we have:
    //      3 * 5ms = 15ms < 1024 frames * 1000 / 48000 = 21.333ms
    static constexpr int MAX_READ_ATTEMPTS = 3;
    // 5ms between two read attempts when pipe is empty
    static constexpr int READ_ATTEMPT_SLEEP_MS = 5;
    static constexpr float kMicrosPerSecond = MICROS_PER_SECOND;
    static constexpr float kMillisPerSecond = MILLIS_PER_SECOND;
};

class StreamInRemoteSubmix final : public StreamIn {
    ndk::ScopedAStatus getActiveMicrophones(
            std::vector<::aidl::android::media::audio::common::MicrophoneDynamicInfo>* _aidl_return)
            override;

  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones,
            std::shared_ptr<StreamIn>* result);

  private:
    friend class ndk::SharedRefBase;
    StreamInRemoteSubmix(
            const ::aidl::android::hardware::audio::common::SinkMetadata& sinkMetadata,
            StreamContext&& context,
            const std::vector<::aidl::android::media::audio::common::MicrophoneInfo>& microphones);
};

class StreamOutRemoteSubmix final : public StreamOut {
  public:
    static ndk::ScopedAStatus createInstance(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo,
            std::shared_ptr<StreamOut>* result);

  private:
    friend class ndk::SharedRefBase;
    StreamOutRemoteSubmix(
            const ::aidl::android::hardware::audio::common::SourceMetadata& sourceMetadata,
            StreamContext&& context,
            const std::optional<::aidl::android::media::audio::common::AudioOffloadInfo>&
                    offloadInfo);
};

}  // namespace aidl::android::hardware::audio::core
