/*
 * Copyright (C) 2019 The Android Open Source Project
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
#include <aidl/Gtest.h>
#include <aidl/Vintf.h>

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <android/binder_enums.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <future>

using aidl::android::hardware::vibrator::BnVibratorCallback;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;
using ndk::enum_range;
using ndk::ScopedAStatus;
using ndk::SharedRefBase;
using ndk::SpAIBinder;
using std::chrono::high_resolution_clock;

const std::vector<Effect> kEffects{enum_range<Effect>().begin(), enum_range<Effect>().end()};
const std::vector<EffectStrength> kEffectStrengths{enum_range<EffectStrength>().begin(),
                                                   enum_range<EffectStrength>().end()};

const std::vector<Effect> kInvalidEffects = {
        static_cast<Effect>(static_cast<int32_t>(kEffects.front()) - 1),
        static_cast<Effect>(static_cast<int32_t>(kEffects.back()) + 1),
};

const std::vector<EffectStrength> kInvalidEffectStrengths = {
        static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.front()) - 1),
        static_cast<EffectStrength>(static_cast<int8_t>(kEffectStrengths.back()) + 1),
};

const std::vector<CompositePrimitive> kCompositePrimitives{enum_range<CompositePrimitive>().begin(),
                                                           enum_range<CompositePrimitive>().end()};

const std::vector<CompositePrimitive> kInvalidPrimitives = {
        static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.front()) - 1),
        static_cast<CompositePrimitive>(static_cast<int32_t>(kCompositePrimitives.back()) + 1),
};

class CompletionCallback : public BnVibratorCallback {
  public:
    CompletionCallback(const std::function<void()>& callback) : mCallback(callback) {}
    ScopedAStatus onComplete() override {
        mCallback();
        return ScopedAStatus::ok();
    }

  private:
    std::function<void()> mCallback;
};

class VibratorAidl : public testing::TestWithParam<std::string> {
  public:
    virtual void SetUp() override {
        vibrator = IVibrator::fromBinder(
                SpAIBinder(AServiceManager_waitForService(GetParam().c_str())));
        ASSERT_NE(vibrator, nullptr);
        ASSERT_TRUE(vibrator->getCapabilities(&capabilities).isOk());
    }

    std::shared_ptr<IVibrator> vibrator;
    int32_t capabilities;
};

TEST_P(VibratorAidl, OnThenOffBeforeTimeout) {
    EXPECT_TRUE(vibrator->on(2000, nullptr /*callback*/).isOk());
    sleep(1);
    EXPECT_TRUE(vibrator->off().isOk());
}

TEST_P(VibratorAidl, OnWithCallback) {
    if (!(capabilities & IVibrator::CAP_PERFORM_CALLBACK)) return;

    std::promise<void> completionPromise;
    std::future<void> completionFuture{completionPromise.get_future()};
    std::shared_ptr<CompletionCallback> callback = SharedRefBase::make<CompletionCallback>(
            [&completionPromise] { completionPromise.set_value(); });
    uint32_t durationMs = 250;
    std::chrono::milliseconds timeout{durationMs * 2};
    EXPECT_TRUE(vibrator->on(durationMs, callback).isOk());
    EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
    EXPECT_TRUE(vibrator->off().isOk());
}

TEST_P(VibratorAidl, OnCallbackNotSupported) {
    if (!(capabilities & IVibrator::CAP_PERFORM_CALLBACK)) {
        std::shared_ptr<CompletionCallback> callback =
                SharedRefBase::make<CompletionCallback>([] {});
        EXPECT_EQ(EX_UNSUPPORTED_OPERATION, vibrator->on(250, callback).getExceptionCode());
    }
}

TEST_P(VibratorAidl, ValidateEffect) {
    std::vector<Effect> supported;
    ASSERT_TRUE(vibrator->getSupportedEffects(&supported).isOk());

    for (Effect effect : kEffects) {
        bool isEffectSupported =
                std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs = 0;
            ScopedAStatus status =
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);

            if (isEffectSupported) {
                EXPECT_TRUE(status.isOk()) << toString(effect) << " " << toString(strength);
                EXPECT_GT(lengthMs, 0);
                usleep(lengthMs * 1000);
            } else {
                EXPECT_EQ(status.getExceptionCode(), EX_UNSUPPORTED_OPERATION)
                        << toString(effect) << " " << toString(strength);
                EXPECT_EQ(lengthMs, 0);
            }
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallback) {
    if (!(capabilities & IVibrator::CAP_PERFORM_CALLBACK)) return;

    std::vector<Effect> supported;
    ASSERT_TRUE(vibrator->getSupportedEffects(&supported).isOk());

    for (Effect effect : kEffects) {
        bool isEffectSupported =
                std::find(supported.begin(), supported.end(), effect) != supported.end();

        for (EffectStrength strength : kEffectStrengths) {
            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            std::shared_ptr<CompletionCallback> callback = SharedRefBase::make<CompletionCallback>(
                    [&completionPromise] { completionPromise.set_value(); });
            int lengthMs = 0;
            ScopedAStatus status = vibrator->perform(effect, strength, callback, &lengthMs);

            if (isEffectSupported) {
                EXPECT_TRUE(status.isOk());
                EXPECT_GT(lengthMs, 0);
            } else {
                EXPECT_EQ(status.getExceptionCode(), EX_UNSUPPORTED_OPERATION);
                EXPECT_EQ(lengthMs, 0);
            }

            if (!status.isOk()) continue;

            std::chrono::milliseconds timeout{lengthMs * 2};
            EXPECT_EQ(completionFuture.wait_for(timeout), std::future_status::ready);
        }
    }
}

TEST_P(VibratorAidl, ValidateEffectWithCallbackNotSupported) {
    if (capabilities & IVibrator::CAP_PERFORM_CALLBACK) return;

    for (Effect effect : kEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            std::shared_ptr<CompletionCallback> callback =
                    SharedRefBase::make<CompletionCallback>([] {});
            int lengthMs;
            ScopedAStatus status = vibrator->perform(effect, strength, callback, &lengthMs);
            EXPECT_EQ(EX_UNSUPPORTED_OPERATION, status.getExceptionCode());
            EXPECT_EQ(lengthMs, 0);
        }
    }
}

TEST_P(VibratorAidl, InvalidEffectsUnsupported) {
    for (Effect effect : kInvalidEffects) {
        for (EffectStrength strength : kEffectStrengths) {
            int32_t lengthMs;
            ScopedAStatus status =
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);
            EXPECT_EQ(status.getExceptionCode(), EX_UNSUPPORTED_OPERATION)
                    << toString(effect) << " " << toString(strength);
        }
    }
    for (Effect effect : kEffects) {
        for (EffectStrength strength : kInvalidEffectStrengths) {
            int32_t lengthMs;
            ScopedAStatus status =
                    vibrator->perform(effect, strength, nullptr /*callback*/, &lengthMs);
            EXPECT_EQ(status.getExceptionCode(), EX_UNSUPPORTED_OPERATION)
                    << toString(effect) << " " << toString(strength);
        }
    }
}

TEST_P(VibratorAidl, ChangeVibrationAmplitude) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_EQ(EX_NONE, vibrator->setAmplitude(0.1f).getExceptionCode());
        EXPECT_TRUE(vibrator->on(2000, nullptr /*callback*/).isOk());
        EXPECT_EQ(EX_NONE, vibrator->setAmplitude(0.5f).getExceptionCode());
        sleep(1);
        EXPECT_EQ(EX_NONE, vibrator->setAmplitude(1.0f).getExceptionCode());
        sleep(1);
    }
}

TEST_P(VibratorAidl, AmplitudeOutsideRangeFails) {
    if (capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) {
        EXPECT_EQ(EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(-1).getExceptionCode());
        EXPECT_EQ(EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(0).getExceptionCode());
        EXPECT_EQ(EX_ILLEGAL_ARGUMENT, vibrator->setAmplitude(1.1).getExceptionCode());
    }
}

TEST_P(VibratorAidl, AmplitudeReturnsUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_AMPLITUDE_CONTROL) == 0) {
        EXPECT_EQ(EX_UNSUPPORTED_OPERATION, vibrator->setAmplitude(1).getExceptionCode());
    }
}

TEST_P(VibratorAidl, ChangeVibrationExternalControl) {
    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_TRUE(vibrator->setExternalControl(true).isOk());
        sleep(1);
        EXPECT_TRUE(vibrator->setExternalControl(false).isOk());
        sleep(1);
    }
}

TEST_P(VibratorAidl, ExternalAmplitudeControl) {
    const bool supportsExternalAmplitudeControl =
            (capabilities & IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL) > 0;

    if (capabilities & IVibrator::CAP_EXTERNAL_CONTROL) {
        EXPECT_TRUE(vibrator->setExternalControl(true).isOk());

        ScopedAStatus amplitudeStatus = vibrator->setAmplitude(0.5);
        if (supportsExternalAmplitudeControl) {
            EXPECT_TRUE(amplitudeStatus.isOk());
        } else {
            EXPECT_EQ(amplitudeStatus.getExceptionCode(), EX_UNSUPPORTED_OPERATION);
        }
        EXPECT_TRUE(vibrator->setExternalControl(false).isOk());
    } else {
        EXPECT_FALSE(supportsExternalAmplitudeControl);
    }
}

TEST_P(VibratorAidl, ExternalControlUnsupportedMatchingCapabilities) {
    if ((capabilities & IVibrator::CAP_EXTERNAL_CONTROL) == 0) {
        EXPECT_EQ(EX_UNSUPPORTED_OPERATION, vibrator->setExternalControl(true).getExceptionCode());
    }
}

TEST_P(VibratorAidl, GetSupportedPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;

        EXPECT_EQ(EX_NONE, vibrator->getSupportedPrimitives(&supported).getExceptionCode());

        std::sort(supported.begin(), supported.end());

        EXPECT_EQ(kCompositePrimitives, supported);
    }
}

TEST_P(VibratorAidl, GetPrimitiveDuration) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t duration;

        for (auto primitive : kCompositePrimitives) {
            EXPECT_EQ(EX_NONE,
                      vibrator->getPrimitiveDuration(primitive, &duration).getExceptionCode());
        }
    }
}

TEST_P(VibratorAidl, ComposeValidPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t maxDelay, maxSize;

        EXPECT_EQ(EX_NONE, vibrator->getCompositionDelayMax(&maxDelay).getExceptionCode());
        EXPECT_EQ(EX_NONE, vibrator->getCompositionSizeMax(&maxSize).getExceptionCode());

        std::vector<CompositeEffect> composite;

        for (auto primitive : kCompositePrimitives) {
            CompositeEffect effect;

            effect.delayMs = std::rand() % (maxDelay + 1);
            effect.primitive = primitive;
            effect.scale = static_cast<float>(std::rand()) / RAND_MAX ?: 1.0f;
            composite.emplace_back(effect);

            if (composite.size() == maxSize) {
                EXPECT_EQ(EX_NONE, vibrator->compose(composite, nullptr).getExceptionCode());
                composite.clear();
                vibrator->off();
            }
        }

        if (composite.size() != 0) {
            EXPECT_EQ(EX_NONE, vibrator->compose(composite, nullptr).getExceptionCode());
            vibrator->off();
        }
    }
}

TEST_P(VibratorAidl, ComposeUnsupportedPrimitives) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        for (auto primitive : kInvalidPrimitives) {
            std::vector<CompositeEffect> composite(1);

            for (auto& effect : composite) {
                effect.delayMs = 0;
                effect.primitive = primitive;
                effect.scale = 1.0f;
            }
            EXPECT_EQ(EX_UNSUPPORTED_OPERATION,
                      vibrator->compose(composite, nullptr).getExceptionCode());
            vibrator->off();
        }
    }
}

TEST_P(VibratorAidl, CompseDelayBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t maxDelay;

        EXPECT_EQ(EX_NONE, vibrator->getCompositionDelayMax(&maxDelay).getExceptionCode());

        std::vector<CompositeEffect> composite(1);
        CompositeEffect effect;

        effect.delayMs = 1;
        effect.primitive = CompositePrimitive::CLICK;
        effect.scale = 1.0f;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(EX_NONE, vibrator->compose(composite, nullptr).getExceptionCode());

        effect.delayMs = maxDelay + 1;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(EX_ILLEGAL_ARGUMENT, vibrator->compose(composite, nullptr).getExceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, CompseSizeBoundary) {
    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        int32_t maxSize;

        EXPECT_EQ(EX_NONE, vibrator->getCompositionSizeMax(&maxSize).getExceptionCode());

        std::vector<CompositeEffect> composite(maxSize);
        CompositeEffect effect;

        effect.delayMs = 1;
        effect.primitive = CompositePrimitive::CLICK;
        effect.scale = 1.0f;

        std::fill(composite.begin(), composite.end(), effect);
        EXPECT_EQ(EX_NONE, vibrator->compose(composite, nullptr).getExceptionCode());

        composite.emplace_back(effect);
        EXPECT_EQ(EX_ILLEGAL_ARGUMENT, vibrator->compose(composite, nullptr).getExceptionCode());
        vibrator->off();
    }
}

TEST_P(VibratorAidl, ComposeCallback) {
    constexpr std::chrono::milliseconds allowedLatency{10};

    if (capabilities & IVibrator::CAP_COMPOSE_EFFECTS) {
        std::vector<CompositePrimitive> supported;

        ASSERT_TRUE(vibrator->getSupportedPrimitives(&supported).isOk());

        for (auto primitive : supported) {
            if (primitive == CompositePrimitive::NOOP) {
                continue;
            }

            std::promise<void> completionPromise;
            std::future<void> completionFuture{completionPromise.get_future()};
            std::shared_ptr<CompletionCallback> callback = SharedRefBase::make<CompletionCallback>(
                    [&completionPromise] { completionPromise.set_value(); });
            CompositeEffect effect;
            std::vector<CompositeEffect> composite;
            int32_t durationMs;
            std::chrono::milliseconds duration;
            std::chrono::time_point<high_resolution_clock> start, end;
            std::chrono::milliseconds elapsed;

            effect.delayMs = 0;
            effect.primitive = primitive;
            effect.scale = 1.0f;
            composite.emplace_back(effect);

            EXPECT_EQ(EX_NONE,
                      vibrator->getPrimitiveDuration(primitive, &durationMs).getExceptionCode())
                    << toString(primitive);
            duration = std::chrono::milliseconds(durationMs);

            EXPECT_EQ(EX_NONE, vibrator->compose(composite, callback).getExceptionCode())
                    << toString(primitive);
            start = high_resolution_clock::now();

            EXPECT_EQ(completionFuture.wait_for(duration + allowedLatency),
                      std::future_status::ready)
                    << toString(primitive);
            end = high_resolution_clock::now();

            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            EXPECT_LE(elapsed.count(), (duration + allowedLatency).count()) << toString(primitive);
            EXPECT_GE(elapsed.count(), (duration - allowedLatency).count()) << toString(primitive);
        }
    }
}

TEST_P(VibratorAidl, AlwaysOn) {
    if (capabilities & IVibrator::CAP_ALWAYS_ON_CONTROL) {
        std::vector<Effect> supported;
        ASSERT_TRUE(vibrator->getSupportedAlwaysOnEffects(&supported).isOk());

        for (Effect effect : kEffects) {
            bool isEffectSupported =
                    std::find(supported.begin(), supported.end(), effect) != supported.end();

            for (EffectStrength strength : kEffectStrengths) {
                ScopedAStatus status = vibrator->alwaysOnEnable(0, effect, strength);

                if (isEffectSupported) {
                    EXPECT_EQ(EX_NONE, status.getExceptionCode())
                            << toString(effect) << " " << toString(strength);
                } else {
                    EXPECT_EQ(EX_UNSUPPORTED_OPERATION, status.getExceptionCode())
                            << toString(effect) << " " << toString(strength);
                }
            }
        }

        EXPECT_EQ(EX_NONE, vibrator->alwaysOnDisable(0).getExceptionCode());
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VibratorAidl);
INSTANTIATE_TEST_SUITE_P(Vibrator, VibratorAidl,
                         testing::ValuesIn(android::getAidlHalInstanceNames(IVibrator::descriptor)),
                         android::PrintInstanceNameToString);

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();
    return RUN_ALL_TESTS();
}
