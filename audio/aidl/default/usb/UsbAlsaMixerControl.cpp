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

#define LOG_TAG "AHAL_UsbAlsaMixerControl"
#include <android-base/logging.h>

#include <math.h>
#include <string>
#include <vector>

#include "UsbAlsaMixerControl.h"

using android::NAME_NOT_FOUND;
using android::OK;
using android::Singleton;
using android::sp;
using android::status_t;

ANDROID_SINGLETON_STATIC_INSTANCE(aidl::android::hardware::audio::core::usb::UsbAlsaMixerControl)

namespace aidl::android::hardware::audio::core::usb {

//-----------------------------------------------------------------------------

MixerControl::MixerControl(struct mixer_ctl* ctl) : mCtl(ctl) {
    mNumValues = mixer_ctl_get_num_values(ctl);
    mMaxValue = mixer_ctl_get_range_min(ctl);
    mMaxValue = mixer_ctl_get_range_max(ctl);
}

unsigned int MixerControl::getNumValues() const {
    return mNumValues;
}

int MixerControl::getMaxValue() const {
    return mMaxValue;
}

int MixerControl::getMinValue() const {
    return mMinValue;
}

int MixerControl::getArray(void* array, size_t count) {
    const std::lock_guard guard(mLock);
    return mixer_ctl_get_array(mCtl, array, count);
}

int MixerControl::setArray(const void* array, size_t count) {
    const std::lock_guard guard(mLock);
    return mixer_ctl_set_array(mCtl, array, count);
}

//-----------------------------------------------------------------------------

// static
const std::map<AlsaMixer::Control, std::vector<AlsaMixer::ControlNamesAndExpectedCtlType>>
        AlsaMixer::kPossibleControls = {
                {AlsaMixer::MASTER_SWITCH, {{"Master Playback Switch", MIXER_CTL_TYPE_BOOL}}},
                {AlsaMixer::MASTER_VOLUME, {{"Master Playback Volume", MIXER_CTL_TYPE_INT}}},
                {AlsaMixer::HW_VOLUME,
                 {{"Headphone Playback Volume", MIXER_CTL_TYPE_INT},
                  {"Headset Playback Volume", MIXER_CTL_TYPE_INT},
                  {"PCM Playback Volume", MIXER_CTL_TYPE_INT}}}};

AlsaMixer::AlsaMixer(struct mixer* mixer) {
    mMixer = mixer;
    for (const auto& [control, possibleCtls] : kPossibleControls) {
        for (const auto& [ctlName, expectedCtlType] : possibleCtls) {
            struct mixer_ctl* ctl = mixer_get_ctl_by_name(mMixer, ctlName.c_str());
            if (ctl != nullptr && mixer_ctl_get_type(ctl) == expectedCtlType) {
                mMixerControls.emplace(control, std::make_unique<MixerControl>(ctl));
                break;
            }
        }
    }
}

AlsaMixer::~AlsaMixer() {
    mixer_close(mMixer);
}

namespace {

int volumeFloatToInteger(float fValue, int maxValue, int minValue) {
    return minValue + ceil((maxValue - minValue) * fValue);
}

float volumeIntegerToFloat(int iValue, int maxValue, int minValue) {
    if (iValue > maxValue) {
        return 1.0f;
    }
    if (iValue < minValue) {
        return 0.0f;
    }
    return ((float)(iValue - minValue)) / (maxValue - minValue);
}

}  // namespace

status_t AlsaMixer::setMasterMuted(bool muted) {
    auto it = mMixerControls.find(AlsaMixer::MASTER_SWITCH);
    if (it == mMixerControls.end()) {
        return NAME_NOT_FOUND;
    }
    const int numValues = it->second->getNumValues();
    std::vector<int> values(numValues, muted ? 0 : 1);
    it->second->setArray(values.data(), numValues);
    return OK;
}

status_t AlsaMixer::setMasterVolume(float volume) {
    auto it = mMixerControls.find(AlsaMixer::MASTER_VOLUME);
    if (it == mMixerControls.end()) {
        return NAME_NOT_FOUND;
    }
    const int numValues = it->second->getNumValues();
    std::vector<int> values(numValues, volumeFloatToInteger(volume, it->second->getMaxValue(),
                                                            it->second->getMinValue()));
    it->second->setArray(values.data(), numValues);
    return OK;
}

status_t AlsaMixer::setVolumes(std::vector<float> volumes) {
    auto it = mMixerControls.find(AlsaMixer::HW_VOLUME);
    if (it == mMixerControls.end()) {
        return NAME_NOT_FOUND;
    }
    const int numValues = it->second->getNumValues();
    const int maxValue = it->second->getMaxValue();
    const int minValue = it->second->getMinValue();
    std::vector<int> values;
    size_t i = 0;
    for (; i < numValues && i < values.size(); ++i) {
        values.emplace_back(volumeFloatToInteger(volumes[i], maxValue, minValue));
    }
    it->second->setArray(values.data(), values.size());
    return OK;
}

std::optional<std::vector<float>> AlsaMixer::getVolumes() const {
    auto it = mMixerControls.find(AlsaMixer::HW_VOLUME);
    if (it == mMixerControls.end()) {
        return std::nullopt;
    }
    const int numValues = it->second->getNumValues();
    std::vector<int> values(numValues);
    if (int err = it->second->getArray(values.data(), numValues); err != 0) {
        LOG(ERROR) << __func__ << ": failed to query volume, err=" << err;
        return std::nullopt;
    }
    const int maxValue = it->second->getMaxValue();
    const int minValue = it->second->getMinValue();
    std::vector<float> fValues;
    fValues.reserve(values.size());
    std::transform(values.begin(), values.end(), std::back_inserter(fValues),
                   [maxValue, minValue](const auto& val) {
                       return volumeIntegerToFloat(val, maxValue, minValue);
                   });
    return fValues;
}

//-----------------------------------------------------------------------------

// static
sp<UsbAlsaMixerControl> UsbAlsaMixerControl::getInstance() {
    return &Singleton<UsbAlsaMixerControl>::getInstance();
}

UsbAlsaMixerControl::UsbAlsaMixerControl() : mSelf(this) {}

void UsbAlsaMixerControl::setDeviceConnectionState(int card, bool masterMuted, float masterVolume,
                                                   bool connected) {
    LOG(DEBUG) << __func__ << ": card=" << card << ", connected=" << connected;
    if (connected) {
        struct mixer* mixer = mixer_open(card);
        if (mixer == nullptr) {
            LOG(ERROR) << __func__ << ": failed to open mixer for card=" << card
                       << " errno=" << errno;
            return;
        }
        auto alsaMixer = std::make_shared<AlsaMixer>(mixer);
        alsaMixer->setMasterMuted(masterMuted);
        alsaMixer->setMasterVolume(masterVolume);
        const std::lock_guard guard(mLock);
        mMixerControls.emplace(card, alsaMixer);
    } else {
        const std::lock_guard guard(mLock);
        mMixerControls.erase(card);
    }
}

void UsbAlsaMixerControl::setMasterMuted(bool muted) {
    auto alsaMixers = getAlsaMixers();
    for (auto it = alsaMixers.begin(); it != alsaMixers.end(); ++it) {
        it->second->setMasterMuted(muted);
    }
}

void UsbAlsaMixerControl::setMasterVolume(float volume) {
    auto alsaMixers = getAlsaMixers();
    for (auto it = alsaMixers.begin(); it != alsaMixers.end(); ++it) {
        it->second->setMasterVolume(volume);
    }
}

status_t UsbAlsaMixerControl::setVolumes(int card, std::vector<float> volumes) {
    auto alsaMixer = getAlsaMixer(card);
    return alsaMixer->setVolumes(volumes);
}

std::optional<std::vector<float>> UsbAlsaMixerControl::getVolumes(int card) {
    auto alsaMixer = getAlsaMixer(card);
    if (alsaMixer == nullptr) {
        return std::nullopt;
    }
    return alsaMixer->getVolumes();
}

std::shared_ptr<AlsaMixer> UsbAlsaMixerControl::getAlsaMixer(int card) {
    const std::lock_guard guard(mLock);
    const auto it = mMixerControls.find(card);
    return it == mMixerControls.end() ? nullptr : it->second;
}

std::map<int, std::shared_ptr<AlsaMixer>> UsbAlsaMixerControl::getAlsaMixers() {
    const std::lock_guard guard(mLock);
    return mMixerControls;
}

}  // namespace aidl::android::hardware::audio::core::usb
