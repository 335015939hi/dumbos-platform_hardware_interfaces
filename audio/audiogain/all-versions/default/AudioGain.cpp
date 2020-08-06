/*
 * Copyright (C) 2020 The Android Open Source Project
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

#define LOG_TAG "AudioGain"

#include <log/log.h>

#include "AudioGain.h"
#include <HidlUtils.h>

#include <hidl/HidlTransportSupport.h>

#include <array>

#include <log/log.h>

#include PATH(android/hardware/audio/FILE_VERSION/IDevicesFactory.h)
#include PATH(android/hardware/audio/FILE_VERSION/IPrimaryDevice.h)

namespace android {
namespace hardware {
namespace audio {
namespace audiogain {
namespace CPP_VERSION {
namespace implementation {

using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGainCallback;
using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGain;
using ::android::hardware::audio::audiogain::CPP_VERSION::Result;
using ::android::hardware::audio::CPP_VERSION::IDevicesFactory;
using ::android::hardware::audio::CPP_VERSION::IPrimaryDevice;
using ::android::hardware::Return;

static inline Result toExtensionResult(::android::hardware::audio::CPP_VERSION::Result status) {
    switch (status) {
        case ::android::hardware::audio::CPP_VERSION::Result::OK:
            return Result::OK;
        case ::android::hardware::audio::CPP_VERSION::Result::NOT_SUPPORTED:
            return Result::INVALID_ARGUMENTS;
        case ::android::hardware::audio::CPP_VERSION::Result::INVALID_ARGUMENTS:
            return Result::INVALID_ARGUMENTS;
        default:
            return Result::INVALID_ARGUMENTS;
    }
}

Return<Result> AudioGain::registerAudioGainCallback(
        const android::sp<IAudioGainCallback> &callback)
{
    //
    // Default behavior is dispatched to primary device
    //
    sp<IDevicesFactory> deviceFactory = IDevicesFactory::getService();
    ::android::hardware::audio::CPP_VERSION::Result status;
    deviceFactory->openPrimaryDevice([&](::android::hardware::audio::CPP_VERSION::Result r,
                                         const sp<IPrimaryDevice> &result) {
        if (r != ::android::hardware::audio::CPP_VERSION::Result::OK) {
            status = ::android::hardware::audio::CPP_VERSION::Result::NOT_SUPPORTED;
            return;
        }
        status = result->registerAudioGainCallback(callback);
    });

    return toExtensionResult(status);
}

Return<Result> AudioGain::unregisterAudioGainCallback(
        const android::sp<IAudioGainCallback> &callback)
{
    //
    // Default behavior is dispatched to primary device
    //
    sp<IDevicesFactory> deviceFactory = IDevicesFactory::getService();
    ::android::hardware::audio::CPP_VERSION::Result status;
    deviceFactory->openPrimaryDevice([&](::android::hardware::audio::CPP_VERSION::Result r,
                                         const sp<IPrimaryDevice> &result) {
        if (r != ::android::hardware::audio::CPP_VERSION::Result::OK) {
            status = ::android::hardware::audio::CPP_VERSION::Result::NOT_SUPPORTED;
            return;
        }
        status = result->unregisterAudioGainCallback(callback);
    });
    return toExtensionResult(status);
}

// this is only for passthrough implementations
IAudioGain* HIDL_FETCH_IAudioGain(const char* /* name */) {
    return new AudioGain();
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audiogain
}  // namespace audio
}  // namespace hardware
}  // namespace android
