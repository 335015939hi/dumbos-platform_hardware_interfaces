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

#include "AudioGain.hpp"
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

using ::android::hardware::audio::CPP_VERSION::IDevicesFactory;
using ::android::hardware::audio::CPP_VERSION::IPrimaryDevice;
using ::android::hardware::audio::CPP_VERSION::Result;
using ::android::hardware::Return;
using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGainCallback;
using ::android::hardware::audio::audiogain::CPP_VERSION::IAudioGain;

static inline ::android::hardware::audio::audiogain::CPP_VERSION::Result toExtensionResult(
        Result status) {
    switch (status) {
        case Result::OK:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::OK;
        case Result::INVALID_ARGUMENTS:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::INVALID_ARGUMENTS;
        case Result::INVALID_STATE:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::INVALID_STATE;
        case Result::NOT_INITIALIZED:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::NOT_INITIALIZED;
        case Result::NOT_SUPPORTED:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::NOT_SUPPORTED;
        default:
            return ::android::hardware::audio::audiogain::CPP_VERSION::Result::INVALID_STATE;
    }
}

Return<::android::hardware::audio::audiogain::CPP_VERSION::Result> AudioGain::subscribe(
        const android::sp<IAudioGainCallback> &callback)
{
    //
    // Default behavior is dispatched to primary device
    //
    sp<IDevicesFactory> deviceFactory = IDevicesFactory::getService();
    Result status;
    deviceFactory->openPrimaryDevice([&](Result r, const sp<IPrimaryDevice> &result) {
        if (r != Result::OK) {
            status = Result::NOT_INITIALIZED;
            return;
        }
        status = result->subscribe(callback);
    });

    return toExtensionResult(status);
}

Return<::android::hardware::audio::audiogain::CPP_VERSION::Result> AudioGain::unsubscribe(
        const android::sp<IAudioGainCallback> &callback)
{
    //
    // Default behavior is dispatched to primary device
    //
    sp<IDevicesFactory> deviceFactory = IDevicesFactory::getService();
    Result status;
    deviceFactory->openPrimaryDevice([&](Result r, const sp<IPrimaryDevice> &result) {
        if (r != Result::OK) {
            status = Result::NOT_INITIALIZED;
            return;
        }
        status = result->unsubscribe(callback);
    });
    return toExtensionResult(status);
}

}  // namespace implementation
}  // namespace CPP_VERSION
}  // namespace audiogain
}  // namespace audio
}  // namespace hardware
}  // namespace android
