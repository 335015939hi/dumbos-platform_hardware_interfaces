/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <aidl/android/hardware/audio/effect/BnEffect.h>
#include <fmq/AidlMessageQueue.h>
#include <cstdlib>
#include <memory>

#include "effect-impl/EffectImpl.h"
#include "effect-impl/EffectUUID.h"

namespace aidl::android::hardware::audio::effect {

class ReverbSwContext final : public EffectContext {
  public:
    ReverbSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setRvRoomLevel(int roomLevel) {
        if (roomLevel < MIN_LEVEL || roomLevel > MAX_LEVEL) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room level
        mRoomLevel = roomLevel;
        return RetCode::SUCCESS;
    }
    int getRvRoomLevel() const { return mRoomLevel; }

    RetCode setRvRoomHfLevel(int roomHfLevel) {
        if (roomHfLevel < MIN_ROOM_HF_LEVEL || roomHfLevel > MAX_ROOM_HF_LEVEL) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new room HF level
        mRoomHfLevel = roomHfLevel;
        return RetCode::SUCCESS;
    }
    int getRvRoomHfLevel() const { return mRoomHfLevel; }

    RetCode setRvDecayTime(int decayTime) {
        if (decayTime < MIN_DECAY_TIME || decayTime > MAX_DECAY_TIME) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay time
        mDecayTime = decayTime;
        return RetCode::SUCCESS;
    }
    int getRvDecayTime() const { return mDecayTime; }

    RetCode setRvDecayHfRatio(int decayHfRatio) {
        if (decayHfRatio < MIN_DECAY_HF_RATIO || decayHfRatio > MAX_DECAY_HF_RATIO) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new decay HF ratio
        mDecayHfRatio = decayHfRatio;
        return RetCode::SUCCESS;
    }
    int getRvDecayHfRatio() const { return mDecayHfRatio; }

    RetCode setRvLevel(int level) {
        if (level < MIN_LEVEL || level > MAX_LEVEL) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new level
        mLevel = level;
        return RetCode::SUCCESS;
    }
    int getRvLevel() const { return mLevel; }

    RetCode setRvDelay(int delay) {
        if (delay < MIN_DELAY || delay > MAX_DELAY) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new delay
        mDelay = delay;
        return RetCode::SUCCESS;
    }
    int getRvDelay() const { return mDelay; }

    RetCode setRvDiffusion(int diffusion) {
        if (diffusion < MIN_DIFFUSION || diffusion > MAX_DIFFUSION) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new diffusion
        mDiffusion = diffusion;
        return RetCode::SUCCESS;
    }
    int getRvDiffusion() const { return mDiffusion; }

    RetCode setRvDensity(int density) {
        if (density < MIN_DENSITY || density > MAX_DENSITY) {
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        // TODO : Add implementation to apply new density
        mDensity = density;
        return RetCode::SUCCESS;
    }
    int getRvDensity() const { return mDensity; }

    RetCode setRvBypass(bool bypass) {
        // TODO : Add implementation to apply new bypass
        mBypass = bypass;
        return RetCode::SUCCESS;
    }
    bool getRvBypass() const { return mBypass; }

  private:
    const int MIN_LEVEL = -6000;
    const int MAX_LEVEL = 0;
    const int MIN_ROOM_HF_LEVEL = -4000;
    const int MAX_ROOM_HF_LEVEL = 0;
    const int MIN_DECAY_TIME = 100;
    const int MAX_DECAY_TIME = 20000;
    const int MIN_DECAY_HF_RATIO = 100;
    const int MAX_DECAY_HF_RATIO = 1000;
    const int MIN_DELAY = 0;
    const int MAX_DELAY = 65;
    const int MIN_DIFFUSION = 0;
    const int MAX_DIFFUSION = 1000;
    const int MIN_DENSITY = 0;
    const int MAX_DENSITY = 1000;

    int mRoomLevel = MIN_LEVEL;            // Default room level
    int mRoomHfLevel = MAX_ROOM_HF_LEVEL;  // Default room hf level
    int mDecayTime = 1000;                 // Default decay time
    int mDecayHfRatio = 500;               // Default decay hf ratio
    int mLevel = MIN_LEVEL;                // Default level
    int mDelay = 40;                       // Default delay
    int mDiffusion = MAX_DIFFUSION;        // Default diffusion
    int mDensity = MAX_DENSITY;            // Default density
    bool mBypass = false;                  // Default bypass
};

class ReverbSw final : public EffectImpl {
  public:
    ReverbSw() { LOG(DEBUG) << __func__; }
    ~ReverbSw() {
        LOG(DEBUG) << __func__;
        releaseContext();
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    IEffect::Status effectProcessImpl(float* in, float* out, int process) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    RetCode releaseContext() override;

  private:
    std::shared_ptr<ReverbSwContext> mContext;
    /* capabilities */
    const int mMaxDecayTime = 20000;
    const Reverb::Capability kCapability = {.maxDecayTimeMs = mMaxDecayTime};
    /* Effect descriptor */
    const Descriptor kDescriptor = {
            .common = {.id = {.type = ReverbTypeUUID,
                              .uuid = ReverbSwImplUUID,
                              .proxy = std::nullopt},
                       .flags = {.type = Flags::Type::INSERT,
                                 .insert = Flags::Insert::FIRST,
                                 .volume = Flags::Volume::CTRL},
                       .name = "ReverbSw"},
            .capability = Capability::make<Capability::reverb>(kCapability)};

    ndk::ScopedAStatus getParameterReverb(const Reverb::Tag& tag, Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
