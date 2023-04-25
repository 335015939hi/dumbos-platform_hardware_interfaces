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

using aidl::android::media::audio::common::AudioChannelLayout;
using aidl::android::media::audio::common::AudioFormatDescription;
using aidl::android::media::audio::common::AudioFormatType;
using aidl::android::media::audio::common::PcmType;
using ::android::MonoPipe;
using ::android::MonoPipeReader;
using ::android::sp;

namespace aidl::android::hardware::audio::core {

namespace r_submix {

static constexpr int kDefaultSampleRateHz = 48000;
// Size at default sample rate
// NOTE: This value will be rounded up to the nearest power of 2 by MonoPipe().
static constexpr int kDefaultPipeSizeInFrames = (1024 * 4);

}  // namespace r_submix

// Configuration of the audio stream.
struct AudioConfig {
    int sampleRate = r_submix::kDefaultSampleRateHz;
    AudioFormatDescription format =
            AudioFormatDescription{.type = AudioFormatType::PCM, .pcm = PcmType::INT_16_BIT};
    AudioChannelLayout channelLayout =
            AudioChannelLayout::make<AudioChannelLayout::Tag::layoutMask>(
                    AudioChannelLayout::LAYOUT_STEREO);
    size_t frameSize;
    size_t frameCount;
};

class SubmixRoute {
  public:
    int id;
    AudioConfig pipeConfig;

    bool isStreamInOpen() {
        std::lock_guard guard(mLock);
        return mStreamInOpen;
    }
    bool getStreamInStandby() {
        std::lock_guard guard(mLock);
        return mStreamInStandby;
    }
    bool isStreamOutOpen() {
        std::lock_guard guard(mLock);
        return mStreamOutOpen;
    }
    bool getStreamOutStandby() {
        std::lock_guard guard(mLock);
        return mStreamOutStandby;
    }
    long getReadCounterFrames() {
        std::lock_guard guard(mLock);
        return mReadCounterFrames;
    }
    int getReadErrorCount() {
        std::lock_guard guard(mLock);
        return mReadErrorCount;
    }
    std::chrono::time_point<std::chrono::steady_clock> getRecordStartTime() {
        std::lock_guard guard(mLock);
        return mRecordStartTime;
    }
    sp<MonoPipe> getSink() {
        std::lock_guard guard(mLock);
        return mSink;
    }
    sp<MonoPipeReader> getSource() {
        std::lock_guard guard(mLock);
        return mSource;
    }

    void init(bool isInput);
    bool isStreamConfigValid(bool isInput, AudioConfig streamConfig);
    void close(bool isInput);
    ::android::status_t createPipe(int portId, AudioConfig streamConfig);
    bool hasStreamOpen();
    int notifyReadError();
    void releasePipe();
    ::android::status_t resetPipe();
    void standby(bool isInput, bool standby);
    long updateReadCounterFrames(size_t frameCount);

  private:
    bool isStreamConfigCompatible(AudioConfig streamConfig);

    std::mutex mLock;

    bool mStreamInOpen GUARDED_BY(mLock) = false;
    int mInputRefCount GUARDED_BY(mLock) = 0;
    bool mStreamInStandby GUARDED_BY(mLock) = true;
    bool mStreamOutStandbyTransition GUARDED_BY(mLock) = false;
    bool mStreamOutOpen GUARDED_BY(mLock) = false;
    bool mStreamOutStandby GUARDED_BY(mLock) = true;
    // how many frames have been requested to be read since standby
    long mReadCounterFrames GUARDED_BY(mLock) = 0;
    int mReadErrorCount GUARDED_BY(mLock) = 0;
    // wall clock when recording starts
    std::chrono::time_point<std::chrono::steady_clock> mRecordStartTime GUARDED_BY(mLock);

    // Pipe variables: they handle the ring buffer that "pipes" audio:
    //  - from the submix virtual audio output == what needs to be played
    //    remotely, seen as an output for the client
    //  - to the virtual audio source == what is captured by the component
    //    which "records" the submix / virtual audio source, and handles it as needed.
    // A usecase example is one where the component capturing the audio is then sending it over
    // Wifi for presentation on a remote Wifi Display device (e.g. a dongle attached to a TV, or a
    // TV with Wifi Display capabilities), or to a wireless audio player.
    sp<MonoPipe> mSink GUARDED_BY(mLock);
    sp<MonoPipeReader> mSource GUARDED_BY(mLock);
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
    size_t getPipeSizeInFrames();
    size_t getStreamPipeSizeInFrames();
    ::android::status_t outWrite(void* buffer, size_t frameCount, size_t* actualFrameCount);
    ::android::status_t inRead(void* buffer, size_t frameCount, size_t* actualFrameCount);

    const int mPortId;
    const bool mIsInput;
    AudioConfig mStreamConfig;
    ::android::status_t mStatus = ::android::NO_INIT;

    // Mutex lock to protect vector of submix routes, each of these submix routes have their mutex
    // locks and none of the mutex locks should be taken together.
    static std::mutex mSubmixRoutesLock;
    static std::vector<std::shared_ptr<SubmixRoute>> mSubmixRoutes GUARDED_BY(mSubmixRoutesLock);

    std::shared_ptr<SubmixRoute> mCurrentRoute;

    // limit for number of read error log entries to avoid spamming the logs
    static constexpr int kMaxReadErrorLogs = 5;
    // The duration of kMaxReadFailureAttempts * READ_ATTEMPT_SLEEP_MS must be strictly inferior
    // to the duration of a record buffer at the current record sample rate (of the device, not of
    // the recording itself). Here we have: 3 * 5ms = 15ms < 1024 frames * 1000 / 48000 = 21.333ms
    static constexpr int kMaxReadFailureAttempts = 3;
    // 5ms between two read attempts when pipe is empty
    static constexpr int kReadAttemptSleepUs = 5000;
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
