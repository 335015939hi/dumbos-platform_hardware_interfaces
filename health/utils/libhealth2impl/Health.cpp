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

#include <health2impl/Health.h>

#include <functional>
#include <string_view>

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android/hardware/health/1.0/types.h>
#include <android/hardware/health/2.0/types.h>
#include <android/hardware/health/2.0/IHealthInfoCallback.h>
#include <android/hardware/health/2.1/IHealthInfoCallback.h>
#include <hal_conversion.h>
#include <healthd/healthd.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

using android::hardware::IPCThreadState;
using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::setupTransportPolling;
using android::hardware::health::V1_0::toString;
using android::hardware::health::V1_0::BatteryStatus;
using android::hardware::health::V2_0::Result;
using android::hardware::health::V1_0::hal_conversion::convertToHealthInfo;
using android::hardware::health::V2_1::IHealth;

using IHealthInfoCallback_2_0 = android::hardware::health::V2_0::IHealthInfoCallback;

namespace android {
namespace hardware {
namespace health {
namespace V2_1 {
namespace implementation {

bool IsDeadObjectLogged(const Return<void>& ret) {
    if (ret.isOk()) return false;
    if (ret.isDeadObject()) return true;
    LOG(ERROR) << "Cannot call healthInfoChanged* on callback: " << ret.description();
    return false;
}

class Callback_2_0 : public Callback {
   public:
    Callback_2_0(const sp<IHealthInfoCallback_2_0>& callback) : callback_(callback) {}
    Return<void> Notify(const HealthInfo& info) override {
        return callback_->healthInfoChanged(info.legacy);
    }
    sp<IBase> Get() override {
        return callback_;
    }
   private:
    sp<IHealthInfoCallback_2_0> callback_;
};

class Callback_2_1 : public Callback {
   public:
    Callback_2_1(const sp<IHealthInfoCallback>& callback) : callback_(callback) {}
    Return<void> Notify(const HealthInfo& info) override {
        return callback_->healthInfoChanged_2_1(info);
    }
    sp<IBase> Get() override {
        return callback_;
    }
   private:
    sp<IHealthInfoCallback> callback_;
};

static std::unique_ptr<Callback> Wrap(const sp<IHealthInfoCallback_2_0>& callback_2_0) {
    auto callback_2_1 = IHealthInfoCallback::castFrom(callback_2_0).withDefault(nullptr);
    if (callback_2_1)
        return std::make_unique<Callback_2_1>(callback_2_1);
    return std::make_unique<Callback_2_0>(callback_2_0);
}

//
// Methods that handle callbacks.
//

Return<Result> Health::registerCallback(const sp<IHealthInfoCallback_2_0>& callback) {
    if (callback == nullptr) {
        return Result::SUCCESS;
    }

    Callback* wrapped = nullptr;
    {
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        wrapped = callbacks_.emplace_back(Wrap(callback)).get();
        // unlock
    }

    auto linkRet = callback->linkToDeath(this, 0u /* cookie */);
    if (!linkRet.withDefault(false)) {
        LOG(WARNING) << __func__ << "Cannot link to death: "
                     << (linkRet.isOk() ? "linkToDeath returns false" : linkRet.description());
        // ignore the error
    }

    getHealthInfo_2_1([&](auto res, const auto& health_info) {
        if (res != Result::SUCCESS) {
            LOG(ERROR) << "Cannot call getHealthInfo_2_1: " << toString(res);
            return;
        }
        auto ret = wrapped->Notify(health_info);
        if (IsDeadObjectLogged(ret)) {
            // Remove callback reference.
            std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
            auto it = std::find_if(callbacks_.begin(), callbacks_.end(), [wrapped](const auto& cb) {
                return cb.get() == wrapped;
            });
            if (it != callbacks_.end()) {
                callbacks_.erase(it);
            }
            // unlock
        }
    });

    return Result::SUCCESS;
}

bool Health::unregisterCallbackInternal(const sp<IBase>& callback) {
    if (callback == nullptr) {
        return false;
    }

    bool removed = false;
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        if (interfacesEqual((*it)->Get(), callback)) {
            it = callbacks_.erase(it);
            removed = true;
        } else {
            ++it;
        }
    }
    (void)callback->unlinkToDeath(this).isOk();  // ignore errors
    return removed;
}

Return<Result> Health::unregisterCallback(const sp<IHealthInfoCallback_2_0>& callback) {
    return unregisterCallbackInternal(callback) ? Result::SUCCESS : Result::NOT_FOUND;
}

void Health::serviceDied(uint64_t /* cookie */, const wp<IBase>& who) {
    (void)unregisterCallbackInternal(who.promote());
}

void Health::NotifyCallbacks(const HealthInfo& health_info) {
    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
    for (auto it = callbacks_.begin(); it != callbacks_.end();) {
        auto ret = (*it)->Notify(health_info);
        if (IsDeadObjectLogged(ret)) {
            it = callbacks_.erase(it);
        } else {
            ++it;
        }
    }
}

//
// Methods that update values. Note that there are two of them, and
// each serves a different purpose. In general:
//
// - A Client calls update() to get a callback immediately.
// - HealthLoop calls ScheduleBatteryUpdate() periodically.
//

// A client requests to schedule health info update and invoke all callbacks.
Return<Result> Health::update() {
    ScheduleBatteryUpdate();
    return Result::SUCCESS;
}

// A client requests to schedule health info update; or HealthLoop periodically
// calls this to invoke all callbacks.
void Health::ScheduleBatteryUpdate() {
    getHealthInfo_2_1([&](auto res, const auto& health_info) {
        if (res != Result::SUCCESS) {
            LOG(ERROR) << "Cannot call getHealthInfo_2_1: " << toString(res);
            return;
        }

        battery_monitor_.logValues();
        bool charger_online = battery_monitor_.isChargerOnline();

        NotifyCallbacks(health_info);

        // adjust uevent / wakealarm periods
        HealthLoop::SetChargerOnline(charger_online);
    });
}

//
// Getters.
//

template <typename T>
static Return<void> GetProperty(BatteryMonitor* monitor, int id, T defaultValue,
                 const std::function<void(Result, T)>& callback) {
    struct BatteryProperty prop;
    T ret = defaultValue;
    Result result = Result::SUCCESS;
    status_t err = monitor->getProperty(static_cast<int>(id), &prop);
    if (err != OK) {
        LOG(DEBUG) << "getProperty(" << id << ")"
                   << " fails: (" << err << ") " << strerror(-err);
    } else {
        ret = static_cast<T>(prop.valueInt64);
    }
    switch (err) {
        case OK:
            result = Result::SUCCESS;
            break;
        case NAME_NOT_FOUND:
            result = Result::NOT_SUPPORTED;
            break;
        default:
            result = Result::UNKNOWN;
            break;
    }
    callback(result, static_cast<T>(ret));
    return Void();
}

Return<void> Health::getChargeCounter(getChargeCounter_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CHARGE_COUNTER, 0, _hidl_cb);
}

Return<void> Health::getCurrentNow(getCurrentNow_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CURRENT_NOW, 0, _hidl_cb);
}

Return<void> Health::getCurrentAverage(getCurrentAverage_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CURRENT_AVG, 0, _hidl_cb);
}

Return<void> Health::getCapacity(getCapacity_cb _hidl_cb) {
    return GetProperty<int32_t>(&battery_monitor_, BATTERY_PROP_CAPACITY, 0, _hidl_cb);
}

Return<void> Health::getEnergyCounter(getEnergyCounter_cb _hidl_cb) {
    return GetProperty<int64_t>(&battery_monitor_, BATTERY_PROP_ENERGY_COUNTER, 0, _hidl_cb);
}

Return<void> Health::getChargeStatus(getChargeStatus_cb _hidl_cb) {
    return GetProperty(&battery_monitor_, BATTERY_PROP_BATTERY_STATUS, BatteryStatus::UNKNOWN, _hidl_cb);
}

Return<void> Health::getStorageInfo(getStorageInfo_cb _hidl_cb) {
    // This implementation does not support StorageInfo. An implementation may extend this
    // class and override this function to support storage info.
    _hidl_cb(Result::NOT_SUPPORTED, {});
    return Void();
}

Return<void> Health::getDiskStats(getDiskStats_cb _hidl_cb) {
    // This implementation does not support DiskStats. An implementation may extend this
    // class and override this function to support disk stats.
    _hidl_cb(Result::NOT_SUPPORTED, {});
    return Void();
}


template <typename T, typename Method>
static inline void GetHealthInfoField(Health* service, Method func, T* out) {
    *out = T{};
    std::invoke(func, service, [out](Result result, const T& value) {
        if (result == Result::SUCCESS) *out = value;
    });
}

Return<void> Health::getHealthInfo(getHealthInfo_cb _hidl_cb) {
    return getHealthInfo_2_1([&](auto res, const auto& health_info) {
        _hidl_cb(res, health_info.legacy);
    });
}

Return<void> Health::getHealthInfo_2_1(getHealthInfo_2_1_cb _hidl_cb) {
    battery_monitor_.updateValues();

    HealthInfo health_info = battery_monitor_.getHealthInfo_2_1();

    // Fill in storage infos; these aren't retrieved by BatteryMonitor.
    GetHealthInfoField(this, &Health::getStorageInfo, &health_info.legacy.storageInfos);
    GetHealthInfoField(this, &Health::getDiskStats, &health_info.legacy.diskStats);

    // A subclass may want to update health info struct before returning it.
    UpdateHealthInfo(&health_info);

    _hidl_cb(Result::SUCCESS, health_info);
    return Void();
}

void Health::UpdateHealthInfo(HealthInfo*) {
    // Sample code for a subclass to implement this:
#if 0
    // If you need to modify values (e.g. batteryChargeTimeToFullNowSeconds), do it here.
    health_info->batteryChargeTimeToFullNowSeconds = calculate_charge_time_seconds();

    // If you need to call healthd_board_battery_update:
    struct BatteryProperties props;
    convertFromHealthInfo(health_info.legacy.legacy, &props);
    healthd_board_battery_update(&props);
    convertToHealthInfo(&props, health_info.legacy.legacy);
#endif
}

Return<void> Health::debug(const hidl_handle& handle, const hidl_vec<hidl_string>&) {
    if (handle == nullptr || handle->numFds == 0) {
        return Void();
    }

    int fd = handle->data[0];
    battery_monitor_.dumpState(fd);
    getHealthInfo_2_1([fd](auto res, const auto& info) {
        android::base::WriteStringToFd("\ngetHealthInfo -> ", fd);
        if (res == Result::SUCCESS) {
            android::base::WriteStringToFd(toString(info), fd);
        } else {
            android::base::WriteStringToFd(toString(res), fd);
        }
        android::base::WriteStringToFd("\n", fd);
    });

    fsync(fd);
    return Void();
}

//
// HealthLoop implementations.
//

void Health::BinderEvent(uint32_t /*epevents*/) {
    if (binder_fd_ >= 0) {
        handleTransportPoll(binder_fd_);
    }
}

void Health::Init(struct healthd_config* config) {
    LOG(INFO) << instance_name_ << " instance initializing with healthd_config...";

    binder_fd_.reset(setupTransportPolling());

    if (binder_fd_ >= 0) {
        auto binder_event = [](auto* health_loop, uint32_t epevents) {
            static_cast<Health*>(health_loop)->BinderEvent(epevents);
        };
        if (!RegisterEvent(binder_fd_, binder_event, EVENT_NO_WAKEUP_FD)) {
            PLOG(ERROR) << instance_name_ << " instance: Register for binder events failed";
        }
    }
    battery_monitor_.init(config);
}

int Health::PrepareToWait(void) {
    IPCThreadState::self()->flushCommands();
    return -1;
}

void Health::Heartbeat(void) {
    // noop
}

// Start the loop in a background thread.
Health::Health(const std::string& name):
    instance_name_(name),
    thread_(std::make_unique<std::thread>(&Health::StartLoop, this)) {}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
