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
#include <media/AidlConversionCppNdk.h>
#include <cmath>

#include <Utils.h>

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
    : mPortId(context.getPortId()), mIsInput(isInput) {
    mStreamConfig.frameSize = context.getFrameSize();
    mStreamConfig.format = context.getFormat();
    mStreamConfig.channelLayout = context.getChannelLayout();
    mStreamConfig.sampleRate = context.getSampleRate();
}

// Verify a submix input or output stream can be opened.
bool DriverRemoteSubmix::isValidConfig() {
    std::lock_guard guard(mCurrentRoute->lock);
    // If the stream is already open, don't open it again.
    // ENABLE_LEGACY_INPUT_OPEN is default behaviour
    if (!mIsInput && mCurrentRoute->outputOpen) {
        LOG(ERROR) << __func__ << ": output stream already open.";
        return false;
    }
    // If either stream is open, verify the existing pipe config matches the stream config.
    if (mCurrentRoute->inputOpen || mCurrentRoute->outputOpen) {
        if (!compareConfigs()) {
            LOG(ERROR) << __func__ << ": Unsupported format.";
            return false;
        }
    }
    return true;
}

// Compare this stream config with existing pipe config, returning false if they do *not*
// match, true otherwise.
bool DriverRemoteSubmix::compareConfigs() {
    AudioConfig pipeConfig = mCurrentRoute->pipeConfig;

    if (mStreamConfig.channelLayout != pipeConfig.channelLayout) {
        LOG(ERROR) << __func__ << ": channel count mismatch, stream channels = "
                   << mStreamConfig.channelLayout.toString()
                   << " pipe config channels = " << pipeConfig.channelLayout.toString();
        return false;
    }
    if (mStreamConfig.sampleRate != pipeConfig.sampleRate) {
        LOG(ERROR) << __func__
                   << ": sample rate mismatch, stream sample rate = " << mStreamConfig.sampleRate
                   << " pipe config sample rate = " << pipeConfig.sampleRate;
        return false;
    }
    if (mStreamConfig.format != pipeConfig.format) {
        LOG(ERROR) << __func__
                   << ": format mismatch, stream format = " << mStreamConfig.format.toString()
                   << " pipe config format = " << pipeConfig.format.toString();
        return false;
    }
    return true;
}

size_t DriverRemoteSubmix::getPipeSizeInFrames() {
    return kDefaultPipeSizeInFrames * ((float)mStreamConfig.sampleRate / kDefaultSampleRateHz);
}

// If one doesn't exist, create a pipe for the submix audio device of size buffer_size_frames and
// optionally associate "in" or "out" with the submix audio device.
::android::status_t DriverRemoteSubmix::createPipe() {
    mCurrentRoute->id = mPortId;

    int channelCount = getChannelCount(mStreamConfig.channelLayout);
    audio_format_t audioFormat = VALUE_OR_RETURN_STATUS(
            aidl2legacy_AudioFormatDescription_audio_format_t(mStreamConfig.format));
    const ::android::NBAIO_Format format =
            ::android::Format_from_SR_C(mStreamConfig.sampleRate, channelCount, audioFormat);
    const ::android::NBAIO_Format offers[1] = {format};
    size_t numCounterOffers = 0;

    size_t pipeSizeInFrames = getPipeSizeInFrames();
    LOG(VERBOSE) << __func__ << ": creating pipe, rate : " << mStreamConfig.sampleRate
                 << ", pipe size : " << pipeSizeInFrames;

    // Create a MonoPipe with optional blocking set to true.
    MonoPipe* sink = new MonoPipe(pipeSizeInFrames, format, true /*writeCanBlock*/);
    if (sink == nullptr) {
        LOG(FATAL) << __func__ << ": sink is null";
    }

    // Negotiation between the source and sink cannot fail as the device open operation
    // creates both ends of the pipe using the same audio format.
    ssize_t index = sink->negotiate(offers, 1, NULL, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the sink failed, index = " << index;
        return ::android::BAD_INDEX;
    }
    MonoPipeReader* source = new MonoPipeReader(sink);
    if (source == nullptr) {
        LOG(FATAL) << __func__ << ": source is null";
    }
    numCounterOffers = 0;
    index = source->negotiate(offers, 1, NULL, numCounterOffers);
    if (index != 0) {
        LOG(FATAL) << __func__ << ": Negotiation for the source failed, index = " << index;
        return ::android::BAD_INDEX;
    }
    LOG(VERBOSE) << __func__ << ": created pipe";

    // Save references to the source and sink.
    mCurrentRoute->sink = sink;
    mCurrentRoute->source = source;

    mCurrentRoute->pipeConfig.frameCount = sink->maxFrames();
    mCurrentRoute->pipeConfig.frameSize = mStreamConfig.frameSize;

    LOG(VERBOSE) << __func__ << ": Pipe frame size : " << mCurrentRoute->pipeConfig.frameSize
                 << ", pipe frames : " << mCurrentRoute->pipeConfig.frameCount;
    return ::android::OK;
}

// Release references to the sink and source.
void DriverRemoteSubmix::releasePipe() {
    mCurrentRoute->sink.clear();
    mCurrentRoute->source.clear();
}

::android::status_t DriverRemoteSubmix::init() {
    {
        std::lock_guard guard(mLock);
        auto submixRouteItr = findPtrById<std::shared_ptr<SubmixRoute>>(mSubmixRoutes, mPortId);
        // If route is not available for this port, add it.
        if (submixRouteItr == mSubmixRoutes.end()) {
            // Initialize the pipe.
            mCurrentRoute = std::make_shared<SubmixRoute>();
            if (::android::OK != createPipe()) {
                LOG(ERROR) << __func__ << ": create pipe failed";
                return mStatus;
            }
            mSubmixRoutes.push_back(mCurrentRoute);
        } else {
            mCurrentRoute = *submixRouteItr;
            if (!isValidConfig()) {
                LOG(ERROR) << __func__ << ": invalid stream config";
                return mStatus;
            }
            sp<MonoPipe> sink = mCurrentRoute->sink;
            if (sink == nullptr) {
                LOG(ERROR) << __func__ << ": nullptr sink when opening stream";
                return mStatus;
            }
            // If the sink has been shutdown or pipe recreation is forced, delete the pipe and
            // recreate it.
            if (sink->isShutdown()) {
                LOG(DEBUG) << __func__ << ": Non-nullptr shut down sink when opening stream";

                releasePipe();
                if (::android::OK != createPipe()) {
                    LOG(ERROR) << __func__ << ": create pipe failed";
                    return mStatus;
                }
            }
        }
    }
    std::lock_guard guard(mCurrentRoute->lock);
    if (mIsInput) {
        if (mCurrentRoute->inputOpen) {
            mCurrentRoute->inputRefCount++;
        } else {
            mCurrentRoute->inputRefCount = 1;
            mCurrentRoute->inputOpen = true;
        }
        mCurrentRoute->inputStandby = true;
        mCurrentRoute->readCounterFrames = 0;
        mCurrentRoute->readErrorCount = 0;
    } else {
        mCurrentRoute->outputOpen = true;
    }
    mCurrentRoute->pipeConfig.channelLayout = mStreamConfig.channelLayout;
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
        sp<MonoPipe> sink = mCurrentRoute->sink;
        if (sink == nullptr) {
            return ::android::UNEXPECTED_NULL;
        }
        if (sink->isShutdown()) {
            return ::android::UNKNOWN_ERROR;
        }
        LOG(DEBUG) << __func__ << ": shutting down MonoPipe sink";

        std::lock_guard guard(mCurrentRoute->lock);
        sink->shutdown(true);
    }
    return ::android::OK;
}

// Remove references to the specified input and output streams.  When the device no longer
// references input and output streams destroy the associated pipe.
::android::status_t DriverRemoteSubmix::close() {
    {
        std::lock_guard guard(mCurrentRoute->lock);
        if (mIsInput) {
            bool shutdown = false;
            mCurrentRoute->inputRefCount--;
            if (mCurrentRoute->inputRefCount == 0) {
                mCurrentRoute->inputOpen = false;
                shutdown = true;
            }
            if (shutdown) {
                sp<MonoPipe> sink = mCurrentRoute->sink;
                if (sink != nullptr) {
                    sink->shutdown(true);
                }
            }
        } else {
            mCurrentRoute->outputOpen = false;
        }
        if (!mCurrentRoute->inputOpen && !mCurrentRoute->outputOpen) {
            releasePipe();
            LOG(DEBUG) << __func__ << ": pipe destroyed";
        }
    }

    bool hasStreamsOpen = true;
    {
        std::lock_guard routeGuard(mCurrentRoute->lock);
        hasStreamsOpen = mCurrentRoute->inputOpen || mCurrentRoute->outputOpen;
    }
    {
        // If all stream instances are closed, we can remove route information for this port.
        if (!hasStreamsOpen) {
            std::lock_guard guard(mLock);
            auto submixRouteItr = findPtrById<std::shared_ptr<SubmixRoute>>(mSubmixRoutes, mPortId);
            mSubmixRoutes.erase(submixRouteItr);
            mStatus = ::android::NO_INIT;
        }
    }
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::transfer(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount, int32_t* latencyMs) {
    {
        std::lock_guard guard(mLock);
        if (mStatus != ::android::OK || mConnectedDevices.empty()) {
            LOG(ERROR) << __func__ << ": failed, not configured, has connected devices: "
                       << mConnectedDevices.empty();
            return ::android::NO_INIT;
        }
    }

    bool isSinkShutdown = false;
    sp<MonoPipe> sink = mCurrentRoute->sink;
    {
        std::lock_guard guard(mCurrentRoute->lock);
        if (sink != nullptr) {
            if (sink->isShutdown()) {
                sink.clear();
                isSinkShutdown = true;
            }
        } else {
            LOG(ERROR) << __func__ << ": transfer without a pipe!";
            return ::android::UNEXPECTED_NULL;
        }
    }

    *latencyMs = (getStreamPipeSizeInFrames() * kMillisPerSecond) / mStreamConfig.sampleRate;
    LOG(VERBOSE) << __func__ << ": Latency " << *latencyMs << "ms";

    if (isSinkShutdown) {
        LOG(VERBOSE) << __func__ << ": pipe shutdown, ignoring the transfer.";
        // the pipe has already been shutdown, this buffer will be lost but we must simulate timing
        // so we don't drain the output faster than realtime
        const size_t delayUs = static_cast<size_t>(
                std::roundf(frameCount * kMicrosPerSecond / mStreamConfig.sampleRate));
        usleep(delayUs);

        *actualFrameCount = frameCount;
        return ::android::OK;
    }

    return (mIsInput ? inRead(buffer, frameCount, actualFrameCount)
                     : outWrite(buffer, frameCount, actualFrameCount));
}

// Calculate the maximum size of the pipe buffer in frames for the specified stream.
size_t DriverRemoteSubmix::getStreamPipeSizeInFrames() {
    auto pipeConfig = mCurrentRoute->pipeConfig;
    const size_t maxFrameSize = std::max(mStreamConfig.frameSize, pipeConfig.frameSize);
    return (pipeConfig.frameCount * pipeConfig.frameSize) / maxFrameSize;
}

::android::status_t DriverRemoteSubmix::outWrite(void* buffer, size_t frameCount,
                                                 size_t* actualFrameCount) {
    bool shutdown = false;
    {
        std::lock_guard guard(mCurrentRoute->lock);
        if (mCurrentRoute->outputStandby) {
            mCurrentRoute->outputStandby = false;
            mCurrentRoute->outputStandbyTransition = true;
        }

        sp<MonoPipe> sink = mCurrentRoute->sink;
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
        const size_t delayUs = static_cast<size_t>(
                std::roundf(frameCount * kMicrosPerSecond / mStreamConfig.sampleRate));
        usleep(delayUs);
        *actualFrameCount = frameCount;
        return ::android::OK;
    }

    // If the write to the sink would block, flush enough frames from the pipe to make space to
    // write the most recent data.
    // We DO NOT block if:
    // - no peer input stream is present
    // - the peer input is in standby AFTER having been active.
    // We DO block if:
    // - the input was never activated to avoid discarding first frames in the pipe in case capture
    // start was delayed
    sp<MonoPipe> sink = mCurrentRoute->sink;
    {
        std::lock_guard guard(mCurrentRoute->lock);
        const size_t availableToWrite = sink->availableToWrite();
        // NOTE: sink has been checked above and sink and source life cycles are synchronized
        sp<MonoPipeReader> source = mCurrentRoute->source;
        const bool shouldBlock =
                mCurrentRoute->inputOpen ||
                (mCurrentRoute->inputStandby && (mCurrentRoute->readCounterFrames != 0));
        if (!shouldBlock && availableToWrite < frameCount) {
            static uint8_t flushBuffer[64];
            const size_t flushBufferSizeFrames = sizeof(flushBuffer) / mStreamConfig.frameSize;
            size_t framesToFlushFromSource = frameCount - availableToWrite;
            LOG(VERBOSE) << __func__ << ": flushing " << framesToFlushFromSource
                         << " frames from the pipe to avoid blocking";
            while (framesToFlushFromSource) {
                const size_t flushSize = std::min(framesToFlushFromSource, flushBufferSizeFrames);
                framesToFlushFromSource -= flushSize;
                // read does not block
                source->read(flushBuffer, flushSize);
            }
        }
    }

    ssize_t writtenFrames = sink->write(buffer, frameCount);

    if (writtenFrames < 0) {
        if (writtenFrames == (ssize_t)::android::NEGOTIATE) {
            LOG(ERROR) << __func__ << ": write to pipe returned NEGOTIATE";
            {
                std::lock_guard guard(mCurrentRoute->lock);
                sink.clear();
            }
            *actualFrameCount = 0;
            return ::android::UNKNOWN_ERROR;
        } else {
            // write() returned UNDERRUN or WOULD_BLOCK, retry
            LOG(ERROR) << __func__ << ": write to pipe returned unexpected " << writtenFrames;
            writtenFrames = sink->write(buffer, frameCount);
        }
    }

    {
        std::lock_guard guard(mCurrentRoute->lock);
        sink.clear();
    }
    if (writtenFrames < 0) {
        LOG(ERROR) << __func__ << ": failed writing to pipe with " << writtenFrames;
        *actualFrameCount = 0;
        return ::android::UNKNOWN_ERROR;
    }
    LOG(VERBOSE) << __func__ << ": wrote " << writtenFrames << "frames";
    *actualFrameCount = writtenFrames;
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::inRead(void* buffer, size_t frameCount,
                                               size_t* actualFrameCount) {
    {
        std::lock_guard guard(mCurrentRoute->lock);
        if (mCurrentRoute->inputStandby || mCurrentRoute->outputStandbyTransition) {
            mCurrentRoute->inputStandby = false;
            mCurrentRoute->outputStandbyTransition = false;
            // keep track of when we exit input standby (== first read == start "real recording")
            // or when we start recording silence, and reset projected time
            mCurrentRoute->recordStartTime = std::chrono::steady_clock::now();
            mCurrentRoute->readCounterFrames = 0;
        }
    }

    mCurrentRoute->readCounterFrames += frameCount;
    size_t remainingFrames = frameCount;

    bool readError = false;
    // about to read from audio source
    sp<MonoPipeReader> source = mCurrentRoute->source;
    {
        std::lock_guard guard(mCurrentRoute->lock);
        if (source == nullptr) {
            if (++mCurrentRoute->readErrorCount < kMaxReadErrorLogs) {
                LOG(ERROR) << __func__
                           << ":no audio pipe yet we're trying to read! (not all errors will be "
                              "logged)";
            } else {
                LOG(ERROR) << __func__ << ":Read errors " << mCurrentRoute->readErrorCount;
            }
            readError = true;
        }
    }

    if (readError) {
        const size_t delayUs = static_cast<size_t>(
                std::roundf(frameCount * kMicrosPerSecond / mStreamConfig.sampleRate));
        usleep(delayUs);
        memset(buffer, 0, mStreamConfig.frameSize * frameCount);
        *actualFrameCount = frameCount;
        return ::android::OK;
    }

    // read the data from the pipe (it's non blocking)
    int attempts = 0;
    const size_t delayUs = static_cast<size_t>(std::roundf(kReadAttemptSleepUs));
    char* buff = (char*)buffer;

    while ((remainingFrames > 0) && (attempts < kMaxReadFailureAttempts)) {
        LOG(VERBOSE) << __func__ << ": frames available to read " << source->availableToRead();

        ssize_t framesRead = source->read(buff, remainingFrames);

        LOG(VERBOSE) << __func__ << ": frames read " << framesRead;

        if (framesRead > 0) {
            remainingFrames -= framesRead;
            buff += framesRead * mStreamConfig.frameSize;
            LOG(VERBOSE) << __func__ << ": (attempts = " << attempts << ") got " << framesRead
                         << " frames, remaining=" << remainingFrames;
        } else {
            attempts++;
            LOG(WARNING) << __func__ << ": read returned " << framesRead
                         << " , read failure attempts = " << attempts;
            usleep(delayUs);
        }
    }

    // done using the source
    {
        std::lock_guard guard(mCurrentRoute->lock);
        source.clear();
    }

    if (remainingFrames > 0) {
        const size_t remainingBytes = remainingFrames * mStreamConfig.frameSize;
        LOG(VERBOSE) << __func__ << ":  clearing remaining_frames = " << remainingFrames;
        memset(((char*)buffer) + (mStreamConfig.frameSize * frameCount) - remainingBytes, 0,
               remainingBytes);
    }

    // compute how much we need to sleep after reading the data by comparing the wall clock with
    //   the projected time at which we should return.
    // wall clock after reading from the pipe
    auto recordDurationUs = std::chrono::steady_clock::now() - mCurrentRoute->recordStartTime;

    // readCounterFrames contains the number of frames that have been read since
    // the beginning of recording (including this call): it's converted to usec and compared to
    // how long we've been recording for, which gives us how long we must wait to sync the
    // projected recording time, and the observed recording time.
    const size_t projectedVsObservedOffsetUs = static_cast<size_t>(std::roundf(
            (mCurrentRoute->readCounterFrames * kMicrosPerSecond / mStreamConfig.sampleRate) -
            recordDurationUs.count()));

    LOG(VERBOSE) << __func__ << ":  record duration " << recordDurationUs.count()
                 << " microseconds, will wait: " << projectedVsObservedOffsetUs << " microseconds";
    if (projectedVsObservedOffsetUs > 0) {
        usleep(projectedVsObservedOffsetUs);
    }

    *actualFrameCount = frameCount;
    return ::android::OK;
}

::android::status_t DriverRemoteSubmix::standby() {
    std::lock_guard guard(mCurrentRoute->lock);
    if (mIsInput) {
        mCurrentRoute->inputStandby = true;
    } else {
        mCurrentRoute->outputStandby = true;
        mCurrentRoute->outputStandbyTransition = !mCurrentRoute->outputStandbyTransition;
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
    LOG(ERROR) << __func__ << ": not supported";
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

}  // namespace aidl::android::hardware::audio::core
