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

#include <android-base/logging.h>

#include "device_port_proxy.h"
#include "BluetoothAudioSessionControl.h"

#include "core-impl/StreamBluetooth.h"

using aidl::android::hardware::audio::common::SinkMetadata;
using aidl::android::hardware::audio::common::SourceMetadata;
using aidl::android::hardware::bluetooth::audio::PcmConfiguration;
using aidl::android::media::audio::common::AudioDevice;
using aidl::android::media::audio::common::AudioDeviceAddress;
using aidl::android::media::audio::common::AudioOffloadInfo;
using aidl::android::media::audio::common::MicrophoneInfo;

using android::bluetooth::audio::aidl::BluetoothAudioPortAidlIn;
using android::bluetooth::audio::aidl::BluetoothAudioPortAidlOut;

namespace aidl::android::hardware::audio::core {

DriverBluetooth::DriverBluetooth(const StreamContext& context, bool isInput)
    : mSampleRate(context.getSampleRate()),
      mChannelLayout(context.getChannelLayout()),
      mFormat(context.getFormat()),
      mFrameSizeBytes(context.getFrameSize()),
      mIsInput(isInput) {}

::android::status_t DriverBluetooth::init() {
    mInitDone = false;  // defering this till we get AudioDevice info
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
    {
        std::lock_guard guard(mLock);
        if (mConnectedDevices.empty()) {
            LOG(ERROR) << __func__ << ", has connected devices: " << mConnectedDevices.empty();
            return ::android::NO_INIT;
        }
    }
    if (!mInitDone) {
        if (::android::status_t status = doInit(); status != ::android::OK) {
            LOG(ERROR) << __func__ << ": encountered error during init, status=" << status;
            return status;
        }
    }
    // TODO: make call to proxy->readData() or proxy->writeData() basing on state
    (void)buffer;
    *actualFrameCount = frameCount;
    *latencyMs = Module::kLatencyMs;
    return ::android::OK;
}

::android::status_t DriverBluetooth::doInit() {
    if (!::aidl::android::hardware::bluetooth::audio::BluetoothAudioSession::IsAidlAvailable()) {
        LOG(ERROR) << __func__ << ": IBluetoothAudioProviderFactory service not available";
        return ::android::UNKNOWN_ERROR;
    }

    std::vector<AudioDevice> connectedDevices;
    {
        std::lock_guard guard(mLock);
        connectedDevices = mConnectedDevices;
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

::android::status_t DriverBluetooth::standby() {
    // TODO: make call to proxy->Suspend() basing on state
    return ::android::OK;
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

}  // namespace aidl::android::hardware::audio::core