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
#include <audio_utils/clock.h>

#include "BluetoothAudioSessionControl.h"
#include "core-impl/StreamBluetooth.h"

namespace aidl::android::hardware::audio::core {

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
using ::aidl::android::hardware::audio::core::VendorParameter;
using ::aidl::android::hardware::bluetooth::audio::ChannelMode;
using ::aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using ::aidl::android::hardware::bluetooth::audio::PresentationPosition;
using ::aidl::android::media::audio::common::AudioChannelLayout;
using ::aidl::android::media::audio::common::AudioDevice;
using ::aidl::android::media::audio::common::AudioDeviceAddress;
using ::aidl::android::media::audio::common::AudioFormatDescription;
using ::aidl::android::media::audio::common::AudioFormatType;
using ::aidl::android::media::audio::common::AudioOffloadInfo;
using ::aidl::android::media::audio::common::MicrophoneDynamicInfo;
using ::aidl::android::media::audio::common::MicrophoneInfo;
using ::android::bluetooth::audio::aidl::BluetoothAudioPort;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using ::android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;
using ::android::bluetooth::audio::aidl::BluetoothStreamState;

constexpr int kBluetoothDefaultInputBufferMs = 20;
constexpr int kBluetoothDefaultOutputBufferMs = 10;
// constexpr int kBluetoothSpatializerOutputBufferMs = 10;

size_t getFrameCount(uint64_t durationUs, uint32_t sampleRate) {
    return (durationUs * sampleRate) / 1000000;
}

// pcm configuration params are not really used by the module
StreamBluetooth::StreamBluetooth(const Metadata& metadata, StreamContext&& context)
    : StreamCommonImpl(metadata, std::move(context)),
      mSampleRate(context.getSampleRate()),
      mChannelLayout(context.getChannelLayout()),
      mFormat(context.getFormat()),
      mFrameSizeBytes(context.getFrameSize()),
      mIsInput(isInput(metadata)) {
    mPreferredDataIntervalUs =
            mIsInput ? kBluetoothDefaultInputBufferMs : kBluetoothDefaultOutputBufferMs;
    mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
    mInitDone = false;
    mIsReadyToClose = false;
}

::android::status_t StreamBluetooth::init() {
    return ::android::OK;  // defering this till we get AudioDeviceDescription
}

ndk::ScopedAStatus StreamBluetooth::setConnectedDevices(
        const std::vector<AudioDevice>& connectedDevices) {
    if (mIsInput && connectedDevices.size() > 1) {
        LOG(ERROR) << __func__ << ": wrong device size(" << connectedDevices.size()
                   << ") for input stream";
        return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
    for (const auto& connectedDevice : connectedDevices) {
        if (connectedDevice.address.getTag() != AudioDeviceAddress::mac) {
            LOG(ERROR) << __func__ << ": bad device address" << connectedDevice.address.toString();
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        }
    }
    std::lock_guard guard(mLock);
    RETURN_STATUS_IF_ERROR(StreamCommonImpl::setConnectedDevices(connectedDevices));
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamBluetooth::drain(StreamDescriptor::DrainMode) {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamBluetooth::flush() {
    usleep(1000);
    return ::android::OK;
}

::android::status_t StreamBluetooth::pause() {
    return standby();
}

::android::status_t StreamBluetooth::transfer(void* buffer, size_t frameCount,
                                              size_t* actualFrameCount, int32_t* latencyMs) {
    std::unique_lock lock(mLock);
    ::android::base::ScopedLockAssertion lock_assertion(mLock);
    if (::android::status_t status = doInit(); status != ::android::OK) return status;
    if (mIsReadyToClose) {  // stream is ready to close, no further transfers
        *actualFrameCount = 0;
        return ::android::OK;
    }
    for (auto proxy : mBtDeviceProxies) {
        if (!proxy->start()) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState() << " failed to start ";
            return -EIO;
        }
        const size_t fc = std::min(frameCount, mPreferredFrameCount);
        const size_t bytesToTransfer = fc * mFrameSizeBytes;
        if (mIsInput) {
            const size_t totalRead = proxy->readData(buffer, bytesToTransfer);
            *actualFrameCount = totalRead / mFrameSizeBytes;
        } else {
            const size_t totalWrite = proxy->writeData(buffer, bytesToTransfer);
            *actualFrameCount = totalWrite / mFrameSizeBytes;
        }
        // CHECK: are all output ports guaranteed same actualFrameCount, latency?
        PresentationPosition presentation_position;
        auto retVal = proxy->getPresentationPosition(presentation_position);
        if (!retVal) {
            LOG(ERROR) << __func__ << ": getPresentationPosition returned error ";
            return ::android::UNKNOWN_ERROR;
        }
        *latencyMs = presentation_position.remoteDeviceAudioDelayNanos / NANOS_PER_MILLISECOND;
    }
    return ::android::OK;
}

::android::status_t StreamBluetooth::doInit() {
    if (mInitDone) return ::android::OK;
    if (mConnectedDevices.empty()) {
        LOG(ERROR) << __func__ << ", has no connected devices";
        return ::android::NO_INIT;
    }
    if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::IsAidlAvailable()) {
        LOG(ERROR) << __func__ << ": IBluetoothAudioProviderFactory service not available";
        return ::android::UNKNOWN_ERROR;
    }
    for (const auto& connectedDevice : mConnectedDevices) {
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
                LOG(ERROR) << __func__ << " checkConfigParams failed";
                return ::android::UNKNOWN_ERROR;
            }
            mBtDeviceProxies.push_back(std::move(proxy));
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
                LOG(ERROR) << __func__ << " checkConfigParams failed";
                return ::android::UNKNOWN_ERROR;
            }
            mBtDeviceProxies.push_back(std::move(proxy));
        }
    }
    mInitDone = true;
    return ::android::OK;
}

bool StreamBluetooth::checkConfigParams(
        ::aidl::android::hardware::bluetooth::audio::PcmConfiguration& config) {
    if ((int)mSampleRate != config.sampleRateHz) {
        LOG(ERROR) << __func__ << ": Sample Rate mismatch, stream val = " << mSampleRate
                   << " hal val = " << config.sampleRateHz;
        return false;
    }
    auto channelCount = aidl::android::hardware::audio::common::getChannelCount(mChannelLayout);
    if ((config.channelMode == ChannelMode::MONO && channelCount != 1) ||
        (config.channelMode == ChannelMode::STEREO && channelCount != 2)) {
        LOG(ERROR) << __func__ << ": Channel count mismatch, stream val = " << channelCount
                   << " hal val = " << toString(config.channelMode);
        return false;
    }
    if (mFormat.type != AudioFormatType::PCM) {
        LOG(ERROR) << __func__ << ": unexpected format type "
                   << aidl::android::media::audio::common::toString(mFormat.type);
        return false;
    }
    int8_t bps = aidl::android::hardware::audio::common::getPcmSampleSizeInBytes(mFormat.pcm) * 8;
    if (bps != config.bitsPerSample) {
        LOG(ERROR) << __func__ << ": bits per sample mismatch, stream val = " << bps
                   << " hal val = " << config.bitsPerSample;
        return false;
    }
    if (config.dataIntervalUs != 0) {
        mPreferredDataIntervalUs = config.dataIntervalUs;
        mPreferredFrameCount = getFrameCount(mPreferredDataIntervalUs, mSampleRate);
    }
    return true;
}

ndk::ScopedAStatus StreamBluetooth::prepareToClose() {
    std::lock_guard guard(mLock);
    mIsReadyToClose = true;
    return ndk::ScopedAStatus::ok();
}

::android::status_t StreamBluetooth::standby() {
    std::unique_lock lock(mLock);
    ::android::base::ScopedLockAssertion lock_assertion(mLock);
    if (::android::status_t status = doInit(); status != ::android::OK) return status;
    for (auto proxy : mBtDeviceProxies) {
        if (!proxy->suspend()) {
            LOG(ERROR) << __func__ << ": state = " << proxy->getState() << " failed to stand by ";
            return -EIO;
        }
    }
    return ::android::OK;
}

void StreamBluetooth::shutdown() {
    std::lock_guard guard(mLock);
    if (!mInitDone) return;
    for (auto proxy : mBtDeviceProxies) {
        proxy->stop();
        proxy->unregisterPort();
    }
    mBtDeviceProxies.clear();
}

bool StreamBluetooth::updateSinkMetadata(const SinkMetadata& sinkMetadata) {
    std::unique_lock lock(mLock);
    ::android::base::ScopedLockAssertion lock_assertion(mLock);
    if (::android::OK != doInit()) return false;
    return mBtDeviceProxies[0]->updateSinkMetadata(sinkMetadata);
}

bool StreamBluetooth::updateSourceMetadata(const SourceMetadata& sourceMetadata) {
    std::unique_lock lock(mLock);
    ::android::base::ScopedLockAssertion lock_assertion(mLock);
    if (::android::OK != doInit()) return false;
    for (auto proxy : mBtDeviceProxies) {
        if (!proxy->updateSourceMetadata(sourceMetadata)) return false;
    }
    return true;
}

StreamInBluetooth::StreamInBluetooth(const SinkMetadata& sinkMetadata, StreamContext&& context,
                                     const std::vector<MicrophoneInfo>& microphones)
    : StreamBluetooth(sinkMetadata, std::move(context)), StreamIn(microphones) {}

ndk::ScopedAStatus StreamInBluetooth::getActiveMicrophones(
        std::vector<MicrophoneDynamicInfo>* _aidl_return __unused) {
    LOG(DEBUG) << __func__ << ": not supported";
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus StreamInBluetooth::updateMetadata(const SinkMetadata& in_sinkMetadata) {
    if (updateSinkMetadata(in_sinkMetadata)) return ndk::ScopedAStatus::ok();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

StreamOutBluetooth::StreamOutBluetooth(const SourceMetadata& sourceMetadata,
                                       StreamContext&& context,
                                       const std::optional<AudioOffloadInfo>& offloadInfo)
    : StreamBluetooth(sourceMetadata, std::move(context)), StreamOut(offloadInfo) {}

ndk::ScopedAStatus StreamOutBluetooth::updateMetadata(const SourceMetadata& in_sourceMetadata) {
    if (updateSourceMetadata(in_sourceMetadata)) return ndk::ScopedAStatus::ok();
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
}

}  // namespace aidl::android::hardware::audio::core