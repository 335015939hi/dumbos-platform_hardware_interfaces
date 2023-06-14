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

#define LOG_TAG "AHAL_StreamBluetooth"

#include <Utils.h>
#include <android-base/logging.h>

#include "BluetoothAudioSessionControl.h"
#include "core-impl/StreamBluetooth.h"

namespace aidl::android::hardware::audio::core {

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioOffloadInfo;
using ::aidl::android::media::audio::common::MicrophoneInfo;
using ::android::bluetooth::audio::aidl::BluetoothAudioPort;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;
using ::android::bluetooth::audio::aidl::BluetoothStreamState;

const int kBluetoothDefaultInputBufferMs = 20;
const int kBluetoothDefaultOutputBufferMs = 10;
// const int kBluetoothSpatializerOutputBufferMs = 10;
const int kBluetoothDefaultInputStateTimeoutMs = 20;

static bool stateTransitionTimeOut(std::shared_ptr<BluetoothAudioPort> proxy,
                                   const BluetoothStreamState& state,
                                   int timeOutMs = kBluetoothDefaultInputStateTimeoutMs) {
    /* Don't loose suspend request, AF will not retry */
    while (proxy->getState() == state) {
        /* Don't block AF forever */
        if (--timeOutMs <= 0) {
            LOG(WARNING) << __func__ << ", can't suspend - stucked in: " << state << " state";
            return false;
        }
    }
    return true;
}

size_t getFrameCount(uint64_t durationUs, uint32_t sampleRate) {
    return (durationUs * sampleRate) / 1000000;
}

// pcm configuration params are not really used by the module
DriverBluetooth::DriverBluetooth(const StreamContext& context, bool isInput)
    : mSampleRate(context.getSampleRate()),
      mChannelLayout(context.getChannelLayout()),
      mFormat(context.getFormat()),
      mFrameSizeBytes(context.getFrameSize()),
      mIsInput(isInput) {
    mPreferredDataIntervalUs =
            mIsInput ? kBluetoothDefaultInputBufferMs : kBluetoothDefaultOutputBufferMs;
    mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
}

::android::status_t DriverBluetooth::init() {
    mInitDone = false;  // defering this till we get AudioDeviceDescription
    return ::android::OK;
}

::android::status_t DriverBluetooth::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    if (mIsInput && connectedDevices.size() > 1) {
        LOG(ERROR) << __func__ << ": wrong device size(" << connectedDevices.size()
                   << ") for input stream";
        return ::android::BAD_VALUE;
    }
    for (const auto& connectedDevice : connectedDevices) {
        if (connectedDevice.address.getTag() != AudioDeviceAddress::mac) {
            LOG(ERROR) << __func__ << ": bad device address" << connectedDevice.address.toString();
            return ::android::BAD_VALUE;
        }
    }
    std::lock_guard guard(mLock);
    mConnectedDevices.clear();
    for (const auto& connectedDevice : connectedDevices) {
        mConnectedDevices.push_back(connectedDevice);
    }
    return ::android::OK;
}

::android::status_t DriverBluetooth::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverBluetooth::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t DriverBluetooth::pause() {
    return standby();
}

::android::status_t DriverBluetooth::transfer(void* buffer, size_t frameCount,
                                              size_t* actualFrameCount, int32_t* latencyMs) {
    if (!mInitDone) {
        if (::android::status_t status = doInit(); status != ::android::OK) {
            LOG(ERROR) << __func__ << ": encountered error during init, status=" << status;
            return status;
        }
    }
    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    for (auto proxy : btDeviceProxies) {
        /* If Suspending, Give some time to suspend */
        if (!stateTransitionTimeOut(proxy, BluetoothStreamState::SUSPENDING)) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                       << " Timed out from suspending ";
            return -EBUSY;
        }

        /* If Starting, Give some time to start up */
        if (!stateTransitionTimeOut(proxy, BluetoothStreamState::STARTING)) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                       << " Timed out from starting ";
            return -EBUSY;
        }

        if (proxy->getState() != BluetoothStreamState::STARTED) {
            if (proxy->getState() == BluetoothStreamState::STANDBY) {
                if (!proxy->start()) {
                    LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                               << " failed to start ";
                    return -EIO;
                }
            } else if (proxy->getState() == BluetoothStreamState::DISABLED) {
                LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                           << " Not allowed to start ";
                return -EINVAL;
            } else if (proxy->getState() == BluetoothStreamState::UNKNOWN) {
                LOG(ERROR) << __func__ << ": state = " << proxy->getState() << " bad state ";
                return -EINVAL;
            }
        }
        const size_t fc = std::min(frameCount, mPreferredFrameCount);
        const size_t bytesToTransfer = fc * mFrameSizeBytes;
        if (mIsInput) {
            const size_t totalRead = proxy->readData(buffer, bytesToTransfer);
            *actualFrameCount = totalRead / mFrameSizeBytes;
        } else {
            const size_t totalWrote = proxy->writeData(buffer, bytesToTransfer);
            *actualFrameCount = totalWrote / mFrameSizeBytes;
        }
        uint64_t delayReportNs = 0;
        uint64_t dispersedBytes = 0;
        struct timespec dispersedTimestamp = {};
        proxy->getPresentationPosition(&delayReportNs, &dispersedBytes, &dispersedTimestamp);
        *latencyMs = delayReportNs / 1000000;
    }
    return ::android::OK;
}

::android::status_t DriverBluetooth::doInit() {
    std::vector<AudioDevice> connectedDevices;
    {
        std::lock_guard guard(mLock);
        if (mConnectedDevices.empty()) {
            LOG(ERROR) << __func__ << ", has connected devices: " << mConnectedDevices.empty();
            return ::android::NO_INIT;
        }
        connectedDevices = mConnectedDevices;
    }

    if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::IsAidlAvailable()) {
        LOG(ERROR) << __func__ << ": IBluetoothAudioProviderFactory service not available";
        return ::android::UNKNOWN_ERROR;
    }

    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    for (const auto& connectedDevice : connectedDevices) {
        if (mIsInput) {
            auto proxy = std::shared_ptr<BluetoothAudioPortAidlIn>();
            if (proxy->registerPort(connectedDevice.type)) {
                LOG(ERROR) << __func__ << ": cannot init HAL";
                return ::android::UNKNOWN_ERROR;
            }
            PcmConfiguration config;
            if (!proxy->loadAudioConfig(&config)) {
                LOG(ERROR) << __func__ << ": state=" << proxy->getState()
                           << " failed to get audio config";
                return ::android::UNKNOWN_ERROR;
            }
            if (!checkConfigParams(config)) {
                LOG(ERROR) << __func__ << " failed";
                return ::android::UNKNOWN_ERROR;
            }
            btDeviceProxies.push_back(std::move(proxy));
        } else {
            auto proxy = std::shared_ptr<BluetoothAudioPortAidlOut>();
            if (proxy->registerPort(connectedDevice.type)) {
                LOG(ERROR) << __func__ << ": cannot init HAL";
                return ::android::UNKNOWN_ERROR;
            }
            PcmConfiguration config;
            if (!proxy->loadAudioConfig(&config)) {
                LOG(ERROR) << __func__ << ": state=" << proxy->getState()
                           << " failed to get audio config";
                return ::android::UNKNOWN_ERROR;
            }
            // TODO: Ensure minimum duration for spatialized output?
            // WAR to support Mono / 16 bits per sample as the Bluetooth stack required
            if (config.channelMode == ChannelMode::MONO && config.bitsPerSample == 16) {
                proxy->forcePcmStereoToMono(true);
                config.channelMode = ChannelMode::STEREO;
                LOG(INFO) << __func__ << ": force channels = to be AUDIO_CHANNEL_OUT_STEREO";
            }
            if (!checkConfigParams(config)) {
                LOG(ERROR) << __func__ << " failed";
                return ::android::UNKNOWN_ERROR;
            }
            btDeviceProxies.push_back(std::move(proxy));
        }
    }
    {
        std::lock_guard guard(mLock);
        mBtDeviceProxies = btDeviceProxies;
    }
    mInitDone = true;
    return ::android::OK;
}

bool DriverBluetooth::checkConfigParams(
        ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config) {
    if ((int)mSampleRate != config.sampleRateHz) {
        LOG(ERROR) << __func__ << ": Sample Rate mismatch, stream val = " << mSampleRate
                   << " hal val = " << config.sampleRateHz;
        return false;
    }
    auto channelCount = aidl::android::hardware::audio::common::getChannelCount(mChannelLayout);
    if ((config.channelMode == ChannelMode::MONO && channelCount != 1) ||
        (config.channelMode == ChannelMode::STEREO && channelCount != 2)) {
        LOG(ERROR) << __func__ << ": Channel count mismatch ";
        return false;
    }
    if (mFormat.type != AudioFormatType::PCM) {
        LOG(ERROR) << __func__ << ": unexpected format type "
                   << aidl::android::media::audio::common::toString(mFormat.type);
        return false;
    }
    int8_t bps = aidl::android::hardware::audio::common::getPcmSampleSizeInBytes(mFormat.pcm) * 8;
    if (bps != config.bitsPerSample) {
        LOG(ERROR) << __func__ << ": bits per sample mismatch, stream val " << bps
                   << " hal val = " << config.bitsPerSample;
        return false;
    }
    if (config.dataIntervalUs != 0) {
        mPreferredDataIntervalUs = config.dataIntervalUs;
        mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
    }
    return true;
}

::android::status_t DriverBluetooth::standby() {
    ::android::status_t status = ::android::OK;
    if (!mInitDone) {
        if (status = doInit(); status != ::android::OK) {
            LOG(ERROR) << __func__ << ": encountered error during init, status=" << status;
            return status;
        }
    }

    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    for (auto proxy : btDeviceProxies) {
        LOG(VERBOSE) << __func__ << ": state=" << proxy->getState() << " being standby (suspend)";

        /* If Suspending, Give some time to suspend */
        if (!stateTransitionTimeOut(proxy, BluetoothStreamState::SUSPENDING)) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                       << " Timed out from suspending ";
            return -EBUSY;
        }

        /* If Starting, Give some time to start up */
        if (!stateTransitionTimeOut(proxy, BluetoothStreamState::STARTING)) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                       << " Timed out from starting ";
            return -EBUSY;
        }

        if (proxy->getState() == BluetoothStreamState::STANDBY) {
            LOG(DEBUG) << __func__ << ": state=" << proxy->getState() << " standby already";
            return ::android::OK;
        }

        if (proxy->getState() == BluetoothStreamState::STARTED) {
            if (!proxy->suspend()) {
                LOG(ERROR) << __func__ << ": state = " << proxy->getState()
                           << " failed to stand by ";
                return -EIO;
            }
        }

        LOG(VERBOSE) << __func__ << ": state=" << proxy->getState()
                     << " standby (suspend) status=" << status;
    }
    return status;
}

::android::status_t DriverBluetooth::close() {
    if (!mInitDone) {
        LOG(VERBOSE) << __func__ << "init not done, nothing to cleanup";
        return ::android::OK;
    }

    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    for (auto proxy : btDeviceProxies) {
        if (proxy->getState() != BluetoothStreamState::DISABLED) {
            proxy->stop();
        }
        proxy->unregisterPort();
        LOG(VERBOSE) << __func__ << ": state=" << proxy->getState() << ", stopped";
    }
    {
        std::lock_guard guard(mLock);
        mBtDeviceProxies.clear();
        mInitDone = false;
    }
    return ::android::OK;
}

void DriverBluetooth::updateMetadata(const SinkMetadata& sinkMetadata) {
    if (!mInitDone) {
        LOG(WARNING) << __func__ << "init not done, cannot update metadata";
        return;
    }

    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    auto in = btDeviceProxies[0];
    in->updateSinkMetadata(sinkMetadata);
}

void DriverBluetooth::updateMetadata(const SourceMetadata& sourceMetadata) {
    if (!mInitDone) {
        LOG(WARNING) << __func__ << "init not done, cannot update metadata";
        return;
    }

    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    for (auto out : btDeviceProxies) {
        out->updateSourceMetadata(sourceMetadata);
    }
}

void DriverBluetooth::setVendorParameters(const std::vector<VendorParameter>& parameters) {
    if (!mInitDone) {
        LOG(WARNING) << __func__ << "init not done, cannot set parameters";
        return;
    }

    if (mIsInput) {
        LOG(WARNING) << __func__ << " not handled";
        return;
    }
    std::vector<std::shared_ptr<BluetoothAudioPort>> btDeviceProxies;
    {
        std::lock_guard guard(mLock);
        btDeviceProxies = mBtDeviceProxies;
    }
    for (auto parameter : parameters) {
        for (auto out : btDeviceProxies) {
            // TODO: Check for A2dpSuspended, LeAudioSuspended and call out->suspend()
        }
    }
}

// static
ndk::ScopedAStatus StreamInBluetooth::createInstance(const SinkMetadata& sinkMetadata,
                                                     StreamContext&& context,
                                                     const std::vector<MicrophoneInfo>& microphones,
                                                     std::shared_ptr<StreamIn>* result) {
    std::shared_ptr<StreamIn> stream = ndk::SharedRefBase::make<StreamInBluetooth>(
            sinkMetadata, std::move(context), microphones);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamInBluetooth::StreamInBluetooth(const SinkMetadata& sinkMetadata, StreamContext&& context,
                                     const std::vector<MicrophoneInfo>& microphones)
    : StreamIn(
              sinkMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverBluetooth(ctx, true /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamInWorker(ctx, driver);
              },
              microphones) {}

ndk::ScopedAStatus StreamInBluetooth::updateMetadata(const SinkMetadata& in_sinkMetadata) {
    auto driver = static_cast<DriverBluetooth*>(mDriver.get());
    driver->updateMetadata(in_sinkMetadata);
    return ndk::ScopedAStatus::ok();
}

// static
ndk::ScopedAStatus StreamOutBluetooth::createInstance(
        const SourceMetadata& sourceMetadata, StreamContext&& context,
        const std::optional<AudioOffloadInfo>& offloadInfo, std::shared_ptr<StreamOut>* result) {
    if (offloadInfo.has_value()) {
        LOG(ERROR) << __func__ << ": offload is not supported";
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::shared_ptr<StreamOut> stream = ndk::SharedRefBase::make<StreamOutBluetooth>(
            sourceMetadata, std::move(context), offloadInfo);
    if (auto status = initInstance(stream); !status.isOk()) {
        return status;
    }
    *result = std::move(stream);
    return ndk::ScopedAStatus::ok();
}

StreamOutBluetooth::StreamOutBluetooth(const SourceMetadata& sourceMetadata,
                                       StreamContext&& context,
                                       const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamOut(
              sourceMetadata, std::move(context),
              [](const StreamContext& ctx) -> DriverInterface* {
                  return new DriverBluetooth(ctx, false /*isInput*/);
              },
              [](const StreamContext& ctx, DriverInterface* driver) -> StreamWorkerInterface* {
                  // The default worker implementation is used.
                  return new StreamOutWorker(ctx, driver);
              },
              offloadInfo) {}

ndk::ScopedAStatus StreamOutBluetooth::updateMetadata(const SourceMetadata& in_sourceMetadata) {
    auto driver = static_cast<DriverBluetooth*>(mDriver.get());
    driver->updateMetadata(in_sourceMetadata);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus StreamOutBluetooth::setVendorParameters(
        const std::vector<VendorParameter>& in_parameters, bool in_async) {
    (void)in_async;
    auto driver = static_cast<DriverBluetooth*>(mDriver.get());
    driver->setVendorParameters(in_parameters);
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::audio::core