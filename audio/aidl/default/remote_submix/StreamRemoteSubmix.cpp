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

#define LOG_TAG "AHAL_StreamRemoteSubmix"
#include <android-base/logging.h>

#include <Utils.h>

#include "RemoteSubmixUtils.h"
#include "core-impl/Module.h"
#include "core-impl/StreamRemoteSubmix.h"

using aidl::android::hardware::audio::common::getChannelCount;
using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::AudioPortExt;
using aidl::android::media::audio::common::MicrophoneDynamicInfo;
using aidl::android::media::audio::common::MicrophoneInfo;
using android::OK;
using android::status_t;

namespace aidl::android::hardware::audio::core {

DriverRemoteSubmix::DriverRemoteSubmix(const StreamContext& context, bool isInput)
    : mFrameSizeBytes(context.getFrameSize()),
      mIsInput(isInput),
      mPortId(context.getPortId()),
      mFormat(context.getFormat()),
      mSampleRate(context.getSampleRate()) {
    mChannels = r_submix::getChannelCountFromChannelMask(mContext.getChannelLayout());
}

// Verify a submix input or output stream can be opened.
bool DriverRemoteSubmix::isValidConfig() {
    // If the stream is already open, don't open it again.
    // ENABLE_LEGACY_INPUT_OPEN is default behaviour
    if (!mIsInput && mSubmixRoute.mOutputOpen) {
        LOG(ERROR) << __func__ << ": output stream already open.";
        return false;
    }
    // If either stream is open, verify the existing audio config of pipe matches the user specified
    // config.
    if (mSubmixRoute.mInputOpen || mSubmixRoute.mOutputOpen) {
        if (!compareConfigs()) {
            LOG(ERROR) << __func__ << ": Unsupported format.";
            return false;
        }
    }
    return true;
}

// Compare this user specified config with existing config of pipe, returning false if they do *not*
// match, true otherwise.
bool DriverRemoteSubmix::compareConfigs() {
    PipeConfig config = mSubmixRoute.config;
    // Get the channel mask of the open device.
    int configChannels = mIsInput ? config.mOutputChannels : config.mInputChannels;

    if (mChannels != configChannels) {
        LOG(ERROR) << __func__
                   << ": channel count mismatch, stream channels = " + mChannels +
                              " pipe config channels = " + configChannels;
        return false;
    }
    if (mSampleRate != config.mSampleRate) {
        LOG(ERROR) << __func__
                   << ": sample rate mismatch, stream sample rate = " + mSampleRate +
                              " pipe config sample rate = " + config.mSampleRate;
        return false;
    }
    if (mFormat != config.mFormat) {
        LOG(ERROR) << __func__
                   << ": format mismatch, stream format = " + mFormat +
                              " pipe config format = " + config.mFormat;
        return false;
    }
    return true;
}

size_t DriverRemoteSubmix::getPipeSizeInFrames() {
    return DEFAULT_PIPE_SIZE_IN_FRAMES * ((float)mSampleRate / DEFAULT_SAMPLE_RATE_HZ);
}

// If one doesn't exist, create a pipe for the submix audio device of size buffer_size_frames and
// optionally associate "in" or "out" with the submix audio device.
::android::status_t DriverRemoteSubmix::createPipe() {
    mSubmixRoute.id = mPortId;

    const NBAIO_Format format = Format_from_SR_C(mSampleRate, mChannels, mFormat);
    const NBAIO_Format offers[1] = {format};
    size_t numCounterOffers = 0;

    size_t pipeSizeInFrames = getPipeSizeInFrames();
    LOG(VERBOSE) << __func__
                 << ": creating pipe, rate : " + mSampleRate + ", pipe size : " + pipeSizeInFrames;

    // Create a MonoPipe with optional blocking set to true.
    MonoPipe* sink = new MonoPipe(pipeSizeInFrames, format, true /*writeCanBlock*/);
    if (sink == nullptr) {
        LOG(FATAL) << __func__ << ": Unexpected null, sink = " + sink;
    }

    // Negotiation between the source and sink cannot fail as the device open operation
    // creates both ends of the pipe using the same audio format.
    ssize_t index = sink->negotiate(offers, 1, NULL, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the sink failed, index = " + index;
        return android::BAD_INDEX;
    }
    MonoPipeReader* source = new MonoPipeReader(sink);
    if (source == nullptr) {
        LOG(FATAL) << __func__ << ": Unexpected null, source = " + source;
    }
    numCounterOffers = 0;
    index = source->negotiate(offers, 1, NULL, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the source failed, index = " + index;
        return android::BAD_INDEX;
    }
    LOG(VERBOSE) << __func__ << ": created pipe";

    // Save references to the source and sink.
    mSubmixRoute.sink = sink;
    mSubmixRoute.source = source;

    mSubmixRoute.config.mBufferSizeFrames = sink->maxFrames();
    mSubmixRoute.config.mBufferPeriodSizeFrames =
            mSubmixRoute.config.mBufferSizeFrames / DEFAULT_PIPE_PERIOD_COUNT;
    mSubmixRoute.config.mPipeFrameSize = mFrameSizeBytes;

    LOG(VERBOSE) << __func__
                 << ": Pipe frame size : " + mSubmixRoute.config.mPipeFrameSize +
                            ", pipe size : " + mSubmixRoute.config.mBufferSizeFrames +
                            ", period size : " + mSubmixRoute.config.mBufferPeriodSizeFrames;
    return android::OK;
}

// Release references to the sink and source.
void DriverRemoteSubmix::releasePipe() {
    mSubmixRoute.sink.clear();
    mSubmixRoute.source.clear();
}

::android::status_t DriverRemoteSubmix::init() {
    std::lock_guard guard(mLock);
    auto submixRouteItr = findById<SubmixRoute>(mSubmixRoutes, mPortId);
    // If route is not available for this port, add it.
    if (submixRouteItr == mSubmixRoutes.end()) {
        // Initialize the pipe.
        submixRoute = new SubmixRoute();
        mSubmixRoute = &submixRoute;
        if (android::OK != createPipe()) {
            LOG(ERROR) << __func__ << ": create pipe failed";
            return mStatus;
        }
        mSubmixRoutes.add(submixRoute);
    } else {
        mSubmixRoute = &(*submixRouteItr);
        if (!isValidConfig()) {
            LOG(ERROR) << __func__ << ": invalid stream config";
            return mStatus;
        }
        sp<MonoPipe> sink = mSubmixRoute.config.sink;
        if (sink == nullptr) {
            LOG(ERROR) << __func__ << ": nullptr sink when opening stream";
            return mStatus;
        }
        // If the sink has been shutdown or pipe recreation is forced, delete the pipe and
        // recreate it.
        if (sink->isShutdown()) {
            LOG(DEBUG) << __func__ << ": Non-nullptr shut down sink when opening stream";

            releasePipe();
            if (android::OK != createPipe()) {
                LOG(ERROR) << __func__ << ": create pipe failed";
                return mStatus;
            }
        }
    }
    if (mIsInput) {
        if (mSubmixRoute.mInputOpen) {
            mSubmixRoute.mInputRefCount++;
        } else {
            mSubmixRoute.mInputRefCount = 1;
            mSubmixRoute.mInputOpen = true;
            mSubmixRoute.config.mInputChannels = mChannels;
        }
        mSubmixRoute.mInputStandby = true;
        mSubmixRoute.mReadCounterFrames = 0;
        mSubmixRoute.mReadErrorCount = 0;
    } else {
        mSubmixRoute.mOutputOpen = true;
        mSubmixRoute.config.mOutputChannels = mChannels;
    }
    mStatus = ::android::OK;
    return mStatus;
}

::android::status_t DriverRemoteSubmix::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    for (const auto& connectedDevice : connectedDevices) {
        if (connectedDevice.address.getTag() != AudioDeviceAddress::id) {
            LOG(ERROR) << __func__ << ": bad device address" << connectedDevice.address.toString();
            return ::android::BAD_VALUE;
        }
    }
    std::lock_guard guard(mLock);
    mConnectedDevices.clear();
    for (const auto& connectedDevice : connectedDevices) {
        mConnectedDevices.push_back(connectedDevice.address);
    }
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::pause() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::prepareToClose() {
    if (!mIsInput) {
        sp<MonoPipe> sink = mSubmixRoute.sink;
        if (sink == nullptr || sink->isShutdown()) {
            return;
        }
        LOG(DEBUG) << __func__ << ": shutting down MonoPipe sink";

        std::lock_guard guard(mSubmixRoute.mLock);
        sink->shutdown(true);
    }
    return android::OK;
}

// Remove references to the specified input and output streams.  When the device no longer
// references input and output streams destroy the associated pipe.
::android::status_t DriverRemoteSubmix::close() {
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        if (mIsInput) {
            bool shutdown = false;
            mSubmixRoute.mInputRefCount--;
            if (mSubmixRoute.mInputRefCount == 0) {
                mSubmixRoute.mInputOpen = false;
                shutdown = true;
            }
            if (shutdown) {
                sp<MonoPipe> sink = mSubmixRoute.sink;
                if (sink != nullptr) {
                    sink->shutdown(true);
                }
            }
        } else {
            mSubmixRoute.mOutputOpen = false;
        }
        if (!mSubmixRoute.mInputOpen && !mSubmixRoute.mOutputOpen) {
            releasePipe(mSubmixRoute);
            LOG(DEBUG) << __func__ << ": pipe destroyed");
        }
    }

    std::lock_guard guard(mSubmixRoute.mLock);
    if (!mSubmixRoute.mInputOpen && !mSubmixRoute.mOutputOpen) {
        auto submixRouteItr = findById<SubmixRoute>(mSubmixRoutes, mPortId);
        mSubmixRoutes.erase(submixRouteItr);
        mStatus = ::android::NO_INIT;
    }
    return android::OK;
}

::android::status_t DriverRemoteSubmix::transfer(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount, int32_t* latencyMs) {
    if (mStatus != ::android::OK; || mConnectedDevices.empty()) {
        LOG(ERROR) << __func__ << ": failed, not configured, has connected devices: "
                   << mConnectedDevices.empty();
        return ::android::NO_INIT;
    }

    bool isSinkShutdown = false;
    sp<MonoPipe> sink = mSubmixRoute.sink;
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        if (sink != nullptr) {
            if (sink->isShutdown()) {
                sink.clear();
                isSinkShutdown = true;
            }
        } else {
            LOG(ERROR) << __func__ << ": transfer without a pipe!";
            return android::UNEXPECTED_NULL;
        }
    }

    *latencyMs = (getStreamPipeSizeInFrames() * 1000) / mSampleRate;
    LOG(VERBOSE) << __func__ << ": Latency " + *latencyMs + "ms";

    if (isSinkShutdown) {
        LOG(VERBOSE) << __func__ << ": pipe shutdown, ignoring the transfer.";
        // the pipe has already been shutdown, this buffer will be lost but we must simulate timing
        // so we don't drain the output faster than realtime
        const size_t delayUs =
                static_cast<size_t>(std::roundf(frameCount * kMicrosPerSecond / mSampleRate));
        usleep(delayUs);

        *actualFrameCount = frameCount;
        return ::android::OK;
    }

    return (mIsInput ? inRead(buffer, frameCount, actualFrameCount)
                     : outWrite(buffer, frameCount, actualFrameCount));
}

// Calculate the maximum size of the pipe buffer in frames for the specified stream.
size_t DriverRemoteSubmix::getStreamPipeSizeInFrames() {
    auto config = mSubmixRoute.config;
    const size_t maxFrameSize = std::max(mFrameSizeBytes, config.mPipeFrameSize);
    return (config.mBufferSizeFrames * config.mPipeFrameSize) / maxFrameSize;
}

::android::status_t DriverRemoteSubmix::outWrite(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount) {
    bool shutdown = false;
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        if (mSubmixRoute.mOutputStandby) {
            mSubmixRoute.mOutputStandby = false;
            mSubmixRoute.mOutputStandbyTransition = true;
        }

        sp<MonoPipe> sink = mSubmixRoute.sink;
        if (sink != nullptr) {
            if (sink->isShutdown()) {
                sink.clear();
                shutdown = true;
            }
        } else {
            LOG(FATAL) << __func__ << ": without a pipe!";
            return ::android::UNKNOWN_ERROR;
        }
    }
    if (shutdown) {
        LOG(VERBOSE) << __func__ << ": pipe shutdown, ignoring the write.";
        // the pipe has already been shutdown, this buffer will be lost but we must
        // simulate timing so we don't drain the output faster than realtime
        const size_t delayUs =
                static_cast<size_t>(std::roundf(frameCount * kMicrosPerSecond / mSampleRate));
        usleep(delayUs);
        *actualFrameCount = frameCount;
        return android::OK;
    }

    // If the write to the sink would block, flush enough frames from the pipe to make space to
    // write the most recent data.
    // We DO NOT block if:
    // - no peer input stream is present
    // - the peer input is in standby AFTER having been active.
    // We DO block if:
    // - the input was never activated to avoid discarding first frames in the pipe in case capture
    // start was delayed
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        const size_t availableToWrite = sink->availableToWrite();
        // NOTE: sink has been checked above and sink and source life cycles are synchronized
        sp<MonoPipeReader> source = mSubmixRoute.source;
        const bool shouldBlock =
                mSubmixRoute.mInputOpen ||
                (mSubmixRoute.mInputStandby && (mSubmixRoute.mReadCounterFrames != 0));
        if (!shouldBlock && availableToWrite < frameCount) {
            static uint8_t flushBuffer[64];
            const size_t flushBufferSizeFrames = sizeof(flushBuffer) / frameSize;
            size_t framesToFlushFromSource = frames - availableToWrite;
            LOG(VERBOSE) << __func__
                         << ": flushing " + framesToFlushFromSource +
                                    " frames from the pipe to avoid blocking";
            while (framesToFlushFromSource) {
                const size_t flushSize = std::min(framesToFlushFromSource, flushBufferSizeFrames);
                framesToFlushFromSource -= flush_size;
                // read does not block
                source->read(flushBuffer, flushSize);
            }
        }
    }

    ssize_t writtenFrames = sink->write(buffer, frameCount);

    if (writtenFrames < 0) {
        if (writtenFrames == (ssize_t)NEGOTIATE) {
            LOG(ERROR) << __func__ << ": write to pipe returned NEGOTIATE";
            {
                std::lock_guard guard(mSubmixRoute.mLock);
                sink.clear();
            }
            *actualFrameCount = 0;
            return ::android::UNKNOWN_ERROR;
        } else {
            // write() returned UNDERRUN or WOULD_BLOCK, retry
            LOG(ERROR) << __func__ << ": write to pipe returned unexpected " + writtenFrames;
            writtenFrames = sink->write(buffer, frameCount);
        }
    }

    {
        std::lock_guard guard(mSubmixRoute.mLock);
        sink.clear();
    }
    if (writtenFrames < 0) {
        LOG(ERROR) << __func__ << ": failed writing to pipe with " + writtenFrames;
        *actualFrameCount = 0;
        return ::android::UNKNOWN_ERROR;
    }
    LOG(VERBOSE) << __func__ << ": wrote " + writtenFrames + "frames";
    *actualFrameCount = writtenFrames;
    return android::OK;
}

::android::status_t DriverRemoteSubmix::inRead(void* buffer, size_t frameCount,
                                               size_t* actualFrameCount) {
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        if (mSubmixRoute.mInputStandby || mSubmixRoute.mOutputStandbyTransition) {
            mSubmixRoute.mInputStandby = false;
            mSubmixRoute.mOutputStandbyTransition = false;
            // keep track of when we exit input standby (== first read == start "real recording")
            // or when we start recording silence, and reset projected time
            mSubmixRoute.mRecordStartTime = std::chrono::steady_clock::now();
            mSubmixRoute.mReadCounterFrames = 0;
        }
    }

    mSubmixRoute.mReadCounterFrames += frameCount;
    size_t remainingFrames = frameCount;

    bool readError = false;
    // about to read from audio source
    sp<MonoPipeReader> source = mSubmixRoute.source;
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        if (source == nullptr) {
            mSubmixRoute.mReadErrorCount++;  // ok if it rolls over
            if (mSubmixRoute.mReadErrorCount < MAX_READ_ERROR_LOGS) {
                LOG(ERROR) << __func__
                           << ":no audio pipe yet we're trying to read! (not all errors will be "
                              "logged)";
            }
            readError = true;
        }
    }

    if (readError) {
        const size_t delayUs =
                static_cast<size_t>(std::roundf(frameCount * kMicrosPerSecond / mSampleRate));
        usleep(delayUs);
        memset(buffer, 0, mFrameSize * frameCount);
        *actualFrameCount = frameCount;
        return android::OK;
    }

    // read the data from the pipe (it's non blocking)
    int attempts = 0;
    char* buff = (char*)buffer;

    while ((remainingFrames > 0) && (attempts < MAX_READ_ATTEMPTS)) {
        ssize_t framesRead = -1977;
        size_t readFrames = remaining_frames;

        LOG(VERBOSE) << __func__ << ": frames available to read " + source->availableToRead();

        framesRead = source->read(buff, readFrames);

        LOG(VERBOSE) << __func__ << ": frames read " + framesRead;

        if (framesRead > 0) {
            remainingFrames -= framesRead;
            buff += framesRead * frameSize;
            LOG(VERBOSE) << __func__
                         << ": (attempts = " + attempts + ") got " + framesRead +
                                    " frames, remaining=" + remainingFrames;
        } else {
            attempts++;
            LOG(ERROR) << __func__ << ": read returned " + framesRead;
            const size_t delayUs =
                    static_cast<size_t>(std::roundf(READ_ATTEMPT_SLEEP_MS * kMillisPerSecond));
            usleep(delayUs);
        }
    }

    // done using the source
    {
        std::lock_guard guard(mSubmixRoute.mLock);
        source.clear();
    }

    if (remainingFrames > 0) {
        const size_t remainingBytes = remainingFrames * mFrameSize;
        LOG(VERBOSE) << __func__ << ":  clearing remaining_frames = " + remainingFrames;
        memset(((char*)buffer) + bytes - remainingBytes, 0, remainingBytes);
    }

    // compute how much we need to sleep after reading the data by comparing the wall clock with
    //   the projected time at which we should return.
    // wall clock after reading from the pipe
    auto recordDurationUs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - mRecordStartTime);
    auto recordDurationS = recordDurationUs.count() / 1000000000;
    auto recordDurationNs = recordDurationUs.count() % 1000000000;

    // mReadCounterFrames contains the number of frames that have been read since
    // the beginning of recording (including this call): it's converted to usec and compared to
    // how long we've been recording for, which gives us how long we must wait to sync the
    // projected recording time, and the observed recording time.
    const size_t projectedVsObservedOffsetUs = static_cast<size_t>(
            std::roundf(((mSubmixRoute.mReadCounterFrames - (recordDurationS * mSampleRate))) *
                                kMicrosPerSecond / mSampleRate -
                        recordDurationNs / NANOS_PER_MICROSECOND));

    LOG(VERBOSE) << __func__
                 << ":  record duration " + recordDurationUs.count() +
                            " nanoseconds, will wait: " + projectedVsObservedOffsetUs +
                            " nanoseconds";
    if (projectedVsObservedOffsetUs > 0) {
        usleep(projectedVsObservedOffsetUs);
    }

    *actualFrameCount = frameCount;
    return android::OK;
}

::android::status_t DriverRemoteSubmix::standby() {
    std::lock_guard guard(mSubmixRoute.mLock);
    if (mIsInput) {
        mSubmixRoute.mInputStandby = true;
    } else {
        mSubmixRoute.mOutputStandby = true;
        mSubmixRoute.mOutputStandbyTransition = !mSubmixRoute.mOutputStandbyTransition;
    }
    return ::android::OK;
}

// static
ndk::ScopedAStatus StreamInRemoteSubmix::createInstance(
        const SinkMetadata& sinkMetadata, StreamContext&& context,
        const std::vector<MicrophoneInfo>& microphones, std::shared_ptr<StreamIn>* result) {
    std::shared_ptr<StreamIn> stream = ndk::SharedRefBase::make<StreamInRemoteSubmix>(
            sinkMetadata, std::move(context), microphones);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamInRemoteSubmix::StreamInRemoteSubmix(const SinkMetadata& sinkMetadata,
                                           StreamContext&& context,
                                           const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(
              sinkMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverRemoteSubmix(ctx, true /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamInWorker(ctx, driver);
              },
              microphones) {}

ndk::ScopedAStatus StreamInRemoteSubmix::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

// static
ndk::ScopedAStatus StreamOutRemoteSubmix::createInstance(
        const SourceMetadata& sourceMetadata, StreamContext&& context,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    if (offloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": offload is not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::shared_ptr<StreamOut> stream = ndk::SharedRefBase::make<StreamOutRemoteSubmix>(
            sourceMetadata, std::move(context), offloadInfo);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamOutRemoteSubmix::StreamOutRemoteSubmix(const SourceMetadata& sourceMetadata,
                                             StreamContext&& context,
                                             const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(
              sourceMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverRemoteSubmix(ctx, false /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamOutWorker(ctx, driver);
              },
              offloadInfo) {}
}

}  // namespace aidl::android::hardware::audio::core
