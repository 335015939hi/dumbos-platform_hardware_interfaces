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

class HapticGeneratorSwContext final : public EffectContext {
  public:
    HapticGeneratorSwContext(int statusDepth, const Parameter::Common& common)
        : EffectContext(statusDepth, common) {
        LOG(DEBUG) << __func__;
    }

    RetCode setHgHapticScale(const HapticGenerator::HapticScale& hapticScale) {
        if (hapticScale.id < 0) {
            LOG(ERROR) << __func__ << " invalid hapticScaleId " << hapticScale.id;
            return RetCode::ERROR_ILLEGAL_PARAMETER;
        }
        mHapticScale = hapticScale;
        return RetCode::SUCCESS;
    }
    HapticGenerator::HapticScale getHgHapticScale() const { return mHapticScale; }

    RetCode setHgVibratorInformation(const HapticGenerator::VibratorInformation& vibratorInfo) {
        mVibratorInformation = vibratorInfo;
        return RetCode::SUCCESS;
    }

    HapticGenerator::VibratorInformation getHgVibratorInformation() const {
        return mVibratorInformation;
    }

  private:
    HapticGenerator::HapticScale mHapticScale = {0, HapticGenerator::VibratorScale::MUTE};
    HapticGenerator::VibratorInformation mVibratorInformation = {0, 0, 0};
};

class HapticGeneratorSw final : public EffectImpl {
  public:
    HapticGeneratorSw() { LOG(DEBUG) << __func__; }
    ~HapticGeneratorSw() {
        cleanUp();
        LOG(DEBUG) << __func__;
    }

    ndk::ScopedAStatus getDescriptor(Descriptor* _aidl_return) override;
    ndk::ScopedAStatus setParameterSpecific(const Parameter::Specific& specific) override;
    ndk::ScopedAStatus getParameterSpecific(const Parameter::Id& id,
                                            Parameter::Specific* specific) override;
    IEffect::Status effectProcessImpl(float* in, float* out, int process) override;
    std::shared_ptr<EffectContext> createContext(const Parameter::Common& common) override;
    RetCode releaseContext() override;

  private:
    std::shared_ptr<HapticGeneratorSwContext> mContext;
    /* capabilities */
    const HapticGenerator::Capability kCapability;
    /* Effect descriptor */
    const Descriptor kDescriptor = {
            .common = {.id = {.type = kHapticGeneratorTypeUUID,
                              .uuid = kHapticGeneratorSwImplUUID,
                              .proxy = std::nullopt},
                       .flags = {.type = Flags::Type::INSERT,
                                 .insert = Flags::Insert::FIRST,
                                 .volume = Flags::Volume::CTRL},
                       .name = "HapticGeneratorSw",
                       .implementor = "The Android Open Source Project"},
            .capability = Capability::make<Capability::hapticGenerator>(kCapability)};

    ndk::ScopedAStatus getParameterHapticGenerator(const HapticGenerator::Tag& tag,
                                                   Parameter::Specific* specific);
};
}  // namespace aidl::android::hardware::audio::effect
