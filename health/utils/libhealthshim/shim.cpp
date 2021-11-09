/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/hardware/health/translate-ndk.h>
#include <health-shim/shim.h>

using ::android::sp;
using ::android::h2a::translate;
using ::android::hardware::Return;
using ::android::hardware::health::V2_0::Result;
using ::android::hardware::health::V2_0::toString;
using ::ndk::ScopedAStatus;
using HidlHealth = ::android::hardware::health::V2_0::IHealth;
using HidlHealthInfoCallback = ::android::hardware::health::V2_0::IHealthInfoCallback;
using HidlHealthInfo = ::android::hardware::health::V2_0::IHealthInfo;

namespace aidl::android::hardware::health {

class HealthInfoCallbackShim : public HidlHealthInfoCallback {
  public:
    explicit HealthInfoCallbackShim(const std::shared_ptr<IHealthInfoCallback>& impl)
        : impl_(impl) {}
    ::android::hardware::Return<void> healthInfoChanged(const HidlHealthInfo& info) {
        // This is a oneway function, so we can't (and shouldn't) check for errors.
        (void)impl_->healthInfoChanged(info);
        return Void();
    }

  private:
    std::shared_ptr<IHealthInfoCallback> impl_;
};

HealthShim::HealthShim(const sp<HidlHealth>& service) : service_(service) {}

ScopedAStatus ResultToStatus(Result result) {
    switch (result) {
        case Result::SUCCESS:
            return ScopedAStatus::ok();
        case Result::NOT_SUPPORTED:
            return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
        case Result::UNKNOWN:
            return ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_UNKNOWN);
        case Result::NOT_FOUND:
            return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        case Result::CALLBACK_DIED:
            return ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_CALLBACK_DIED);
    }
    LOG(FATAL) << "Unrecognized result: " << toString(result);
}

ScopedAStatus ReturnResultToStatus(const Return<Result>& return_result) {
    if (return_result.isOk()) {
        return ResultToStatus(return_result);
    }
    if (return_result.isDeadObject()) {
        return ScopedAStatus::fromStatus(STATUS_DEAD_OBJECT);
    }
    return ScopedAStatus::fromServiceSpecificErrorWithMessage(IHealth::STATUS_UNKNOWN,
                                                              return_result.description());
}

ScopedAStatus HealthShim::registerCallback(
        const std::shared_ptr<IHealthInfoCallback>& in_callback) {
    sp<HidlHealthInfoCallback> shim(new HidlHealthInfoCallback(in_callback));
    callback_map_.emplace(in_callback, shim);
    return ReturnResultToStatus(service_->registerCallback(shim));
}

ScopedAStatus HealthShim::unregisterCallback(
        const std::shared_ptr<IHealthInfoCallback>& in_callback) {
    auto it = callback_map_.find(in_callback);
    if (it == callback_map_.end()) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    sp<HidlHealthInfoCallback> shim = it->second;
    callback_map_.erase(it);
    return ReturnResultToStatus(service_->unregisterCallback(shim));
}

ScopedAStatus HealthShim::update() {
    return service_->update();
}


ScopedAStatus HealthShim::getChargeCounterUah(int32_t* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getChargeCounter([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = value;
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getCurrentNowMicroamps(int32_t* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getCurrentNow([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = value;
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getCurrentAverageMicroamps(int32_t* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getCurrentAverage([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = value;
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getCapacity(int32_t* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getCapacity([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = value;
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getEnergyCounterNwh(int64_t* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getEnergyCounter([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = value;
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getChargeStatus(BatteryStatus* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getChargeStatus([out, &out_result](auto result, auto value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = reinterpret_cast<BatteryStatus>(value);
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getStorageInfo(std::vector<StorageInfo>* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getStorageInfo([out, &out_result](auto result, const auto& value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            out->clear();
            out->reserve(value.size());
            for (const auto& e : value) out->push_back(translate(e));
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getDiskStats(std::vector<DiskStats>* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getDiskStats([out, &out_result](auto result, const auto& value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            out->clear();
            out->reserve(value.size());
            for (const auto& e : value) out->push_back(translate(e));
        }
    });
    return ResultToStatus(out_result);
}

ScopedAStatus HealthShim::getHealthInfo(HealthInfo* out) {
    Result out_result = Result::UNKNOWN_ERROR;
    auto ret = service_->getHealthInfo([out, &out_result](auto result, const auto& value) {
        out_result = result;
        if (out_result == Result::SUCCESS) {
            *out = tranlsate(value);
        }
    });
    return ResultToStatus(out_result);
}

}  // namespace aidl::android::hardware::health
