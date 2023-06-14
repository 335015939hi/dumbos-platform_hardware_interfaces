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
#include "DevicePortProxy.h"

#include "core-impl/StreamBluetooth.h"

namespace aidl::android::hardware::audio::core {

using ::aidl::android::hardware::audio::common::SinkMetadata;
using ::aidl::android::hardware::audio::common::SourceMetadata;
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
    while (proxy->GetState() == state) {
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
    usleep(1000);
    return ::android::OK;
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
    if (mIsInput) {
        auto in = btDeviceProxies[0];

        /* Give some time to start up */
        if (!stateTransitionTimeOut(in, BluetoothStreamState::STARTING)) return -EBUSY;

        if (in->GetState() != BluetoothStreamState::STARTED) {
            int retVal = 0;
            if (in->GetState() == BluetoothStreamState::STANDBY) {
                retVal = in->Start() ? 0 : -EIO;
            } else if (in->GetState() == BluetoothStreamState::SUSPENDING) {
                LOG(ERROR) << __func__ << ": state = " << in->GetState() << " Not ready to start ";
                return -EBUSY;
            } else if (in->GetState() == BluetoothStreamState::DISABLED) {
                LOG(ERROR) << __func__ << ": state = " << in->GetState()
                           << " Not allowed to start ";
                return -EINVAL;
            } else if (in->GetState() == BluetoothStreamState::UNKNOWN) {
                LOG(ERROR) << __func__ << ": state = " << in->GetState() << " bad state ";
                return -EINVAL;
            }
            if (retVal) {
                LOG(ERROR) << __func__ << ": state = " << in->GetState() << " failed to start ";
                return retVal;
            }
        }
        const size_t fc = std::min(frameCount, mPreferredFrameCount);
        const size_t bytesToTransfer = fc * mFrameSizeBytes;
        const size_t totalRead = in->ReadData(buffer, bytesToTransfer);
        *actualFrameCount = totalRead / mFrameSizeBytes;
        mFramesProcessed += *actualFrameCount;
        uint64_t delayReportNs = 0;
        uint64_t dispersedBytes = 0;
        struct timespec dispersedTimestamp = {};
        in->GetPresentationPosition(&delayReportNs, &dispersedBytes, &dispersedTimestamp);
        *latencyMs = delayReportNs / 1000000;
    } else {
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
            if (proxy->SetUp(connectedDevice.type)) {
                LOG(ERROR) << __func__ << ": cannot init HAL";
                return ::android::UNKNOWN_ERROR;
            }
            PcmConfiguration config;
            if (!proxy->LoadAudioConfig(&config)) {
                LOG(ERROR) << __func__ << ": state=" << proxy->GetState()
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
            if (proxy->SetUp(connectedDevice.type)) {
                LOG(ERROR) << __func__ << ": cannot init HAL";
                return ::android::UNKNOWN_ERROR;
            }
            PcmConfiguration config;
            if (!proxy->LoadAudioConfig(&config)) {
                LOG(ERROR) << __func__ << ": state=" << proxy->GetState()
                           << " failed to get audio config";
                return ::android::UNKNOWN_ERROR;
            }
            // WAR to support Mono / 16 bits per sample as the Bluetooth stack required
            if (config.channelMode == ChannelMode::STEREO && config.bitsPerSample == 16) {
                proxy->ForcePcmStereoToMono(true);
                LOG(INFO) << __func__ << ": force channels = to be AUDIO_CHANNEL_OUT_STEREO";
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
    if (mIsInput) {
        auto in = btDeviceProxies[0];

        LOG(VERBOSE) << __func__ << ": state=" << in->GetState() << " being standby (suspend)";

        if (in->GetState() == BluetoothStreamState::SUSPENDING) {
            LOG(DEBUG) << __func__ << ": state=" << in->GetState() << " standby already";
            return status;
        }

        /* Give some time to start up */
        if (!stateTransitionTimeOut(in, BluetoothStreamState::STARTING)) {
            LOG(ERROR) << __func__ << ": state=" << in->GetState() << " NOT ready to standby";
            return -EBUSY;
        }

        if (in->GetState() == BluetoothStreamState::STARTED) {
            status = (in->Suspend() ? 0 : -EIO);
        }

        /* Give some time to suspend */
        if (status == ::android::OK &&
            !stateTransitionTimeOut(in, BluetoothStreamState::SUSPENDING)) {
            LOG(ERROR) << __func__ << ": state=" << in->GetState() << " NOT ready to by standby";
            return -EBUSY;
        }

        LOG(VERBOSE) << __func__ << ": state=" << in->GetState()
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
        if (proxy->GetState() != BluetoothStreamState::DISABLED) {
            proxy->Stop();
        }
        proxy->TearDown();
        LOG(VERBOSE) << __func__ << ": state=" << proxy->GetState() << ", stopped";
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
    in->UpdateSinkMetadata(sinkMetadata);
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
        out->UpdateSourceMetadata(sourceMetadata);
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

ndk::ScopedAStatus StreamInBluetooth::updateMetadata(
        const ::aidl::android::hardware::audio::common::SinkMetadata& in_sinkMetadata) {
    auto driver = static_cast<DriverBluetooth*>(mDriver.get());
    driver->updateMetadata(in_sinkMetadata);
    return ndk::ScopedAStatus::ok();
};

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

ndk::ScopedAStatus StreamOutBluetooth::updateMetadata(
        const ::aidl::android::hardware::audio::common::SourceMetadata& in_sourceMetadata) {
    (void)in_sourceMetadata;
    // TODO: make call to proxy->UpdateSourceMetadata()
    return ndk::ScopedAStatus::ok();
};

}  // namespace aidl::android::hardware::audio::core