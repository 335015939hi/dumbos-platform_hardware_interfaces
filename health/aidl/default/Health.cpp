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

#include "health-impl/Health.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android/hardware/health/translate-ndk.h>
#include <health/utils.h>

#include <health-impl/health-convert.h>

namespace aidl::android::hardware::health {

/*
// If you need to call healthd_board_init, construct the Health instance with
// the healthd_config after calling healthd_board_init:
class MyHealth : public Health {
  public:
    MyHealth(std::unique_ptr<healthd_config>&& config) :
        Health(InitConfig(std::move(config))) {}
  private:
    static std::unique_ptr<healthd_config> InitConfig(std::unique_ptr<healthd_config>&& config) {
      healthd_board_init(config.get());
      return std::move(config);
    }
};
*/
Health::Health(const std::string& name, std::unique_ptr<healthd_config>&& config)
    : HalHealthLoop(name),
      healthd_config_(std::move(config)),
      death_recipient_(std::make_unique<DeathRecipient>()) {
    battery_monitor_.init(healthd_config_.get());
}

//
// Getters.
//

template <typename T>
static ndk::ScopedAStatus GetProperty(::android::BatteryMonitor* monitor, int id, T defaultValue,
                                      T* out) {
    *out = defaultValue;
    struct ::android::BatteryProperty prop;
    ::android::status_t err = monitor->getProperty(static_cast<int>(id), &prop);
    if (err == ::android::OK) {
        *out = static_cast<T>(prop.valueInt64);
    } else {
        LOG(DEBUG) << "getProperty(" << id << ")"
                   << " fails: (" << err << ") " << ::android::statusToString(err);
    }

    switch (err) {
        case ::android::OK:
            return ndk::ScopedAStatus::ok();
        case ::android::NAME_NOT_FOUND:
            return ndk::ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_NOT_SUPPORTED);
        default:
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IHealth::STATUS_UNKNOWN, ::android::statusToString(err).c_str());
    }
}

ndk::ScopedAStatus Health::getChargeCounter(int32_t* out) {
    return GetProperty<int32_t>(&battery_monitor_, ::android::BATTERY_PROP_CHARGE_COUNTER, 0, out);
}

ndk::ScopedAStatus Health::getCurrentNow(int32_t* out) {
    return GetProperty<int32_t>(&battery_monitor_, ::android::BATTERY_PROP_CURRENT_NOW, 0, out);
}

ndk::ScopedAStatus Health::getCurrentAverage(int32_t* out) {
    return GetProperty<int32_t>(&battery_monitor_, ::android::BATTERY_PROP_CURRENT_AVG, 0, out);
}

ndk::ScopedAStatus Health::getCapacity(int32_t* out) {
    return GetProperty<int32_t>(&battery_monitor_, ::android::BATTERY_PROP_CAPACITY, 0, out);
}

ndk::ScopedAStatus Health::getEnergyCounter(int64_t* out) {
    return GetProperty<int64_t>(&battery_monitor_, ::android::BATTERY_PROP_ENERGY_COUNTER, 0, out);
}

ndk::ScopedAStatus Health::getChargeStatus(BatteryStatus* out) {
    return GetProperty(&battery_monitor_, ::android::BATTERY_PROP_BATTERY_STATUS,
                       BatteryStatus::UNKNOWN, out);
}

ndk::ScopedAStatus Health::getDiskStats(std::vector<DiskStats>*) {
    // This implementation does not support DiskStats. An implementation may extend this
    // class and override this function to support disk stats.
    return ndk::ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_NOT_SUPPORTED);
}

ndk::ScopedAStatus Health::getStorageInfo(std::vector<StorageInfo>*) {
    // This implementation does not support StorageInfo. An implementation may extend this
    // class and override this function to support storage info.
    return ndk::ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_NOT_SUPPORTED);
}

ndk::ScopedAStatus Health::getHealthInfo(HealthInfo* out) {
    battery_monitor_.updateValues();

    // TODO(b/177269435): BatteryMonitor should store AIDL HealthInfo instead.
    auto health_info_2_1 = battery_monitor_.getHealthInfo_2_1();
    if (!::android::h2a::translate(health_info_2_1, out)) {
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IHealth::STATUS_UNKNOWN, "Cannot translate HIDL HealthInfo to AIDL");
    }

    // Fill in storage infos; these aren't retrieved by BatteryMonitor.
    if (auto res = getStorageInfo(&out->storageInfos); !res.isOk()) {
        if (res.getServiceSpecificError() == 0) {
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IHealth::STATUS_UNKNOWN,
                    ("getStorageInfo fails: " + res.getDescription()).c_str());
        }
        LOG(DEBUG) << "getHealthInfo: getStorageInfo fails with service-specific error, clearing: "
                   << res.getDescription();
        out->storageInfos = {};
    }
    if (auto res = getDiskStats(&out->diskStats); !res.isOk()) {
        if (res.getServiceSpecificError() == 0) {
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IHealth::STATUS_UNKNOWN,
                    ("getDiskStats fails: " + res.getDescription()).c_str());
        }
        LOG(DEBUG) << "getHealthInfo: getDiskStats fails with service-specific error, clearing: "
                   << res.getDescription();
        out->diskStats = {};
    }

    // A subclass may want to update health info struct before returning it.
    UpdateHealthInfo(out);

    return ndk::ScopedAStatus::ok();
}

binder_status_t Health::dump(int fd, const char**, uint32_t) {
    battery_monitor_.dumpState(fd);

    ::android::base::WriteStringToFd("\ngetHealthInfo -> ", fd);
    HealthInfo health_info;
    auto res = getHealthInfo(&health_info);
    if (res.isOk()) {
        ::android::base::WriteStringToFd(health_info.toString(), fd);
    } else {
        ::android::base::WriteStringToFd(res.getDescription(), fd);
    }

    fsync(fd);
    return STATUS_OK;
}

namespace {
bool IsDeadObjectLogged(const ndk::ScopedAStatus& ret) {
    if (ret.isOk()) return false;
    if (ret.getStatus() == ::STATUS_DEAD_OBJECT) return true;
    LOG(ERROR) << "Cannot call healthInfoChanged on callback: " << ret.getDescription();
    return false;
}
}  // namespace

//
// Subclass helpers / overrides
//

void Health::UpdateHealthInfo(HealthInfo* /* health_info */) {
    /*
        // Sample code for a subclass to implement this:
        // If you need to modify values (e.g. batteryChargeTimeToFullNowSeconds), do it here.
        health_info->batteryChargeTimeToFullNowSeconds = calculate_charge_time_seconds();

        // If you need to call healthd_board_battery_update, modify its signature
        // and implementation to operate on HealthInfo directly, then call:
        healthd_board_battery_update(health_info);
    */
}

//
// Methods that handle callbacks.
//

ndk::ScopedAStatus Health::registerCallback(
        const std::shared_ptr<IHealthInfoCallback>& callback) {
    if (callback == nullptr) {
        return ndk::ScopedAStatus::ok();
    }

    {
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        callbacks_.emplace_back(this, callback);
        // unlock
    }

    HealthInfo health_info;
    if (auto res = getHealthInfo(&health_info); !res.isOk()) {
        LOG(WARNING) << "Cannot call getHealthInfo: " << res.getDescription();
        // No health info to send, so return early.
        return ndk::ScopedAStatus::ok();
    }

    if (auto res = callback->healthInfoChanged(health_info); IsDeadObjectLogged(res)) {
        (void)unregisterCallback(callback);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Health::unregisterCallback(
        const std::shared_ptr<IHealthInfoCallback>& callback) {
    if (callback == nullptr) {
        return ndk::ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_NOT_FOUND);
    }

    std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);

    auto matches = [callback](const auto& linked) {
        return linked.callback() == callback.get();  // compares shared_ptr
    };
    auto it = std::remove_if(callbacks_.begin(), callbacks_.end(), matches);
    bool removed = (it != callbacks_.end());
    callbacks_.erase(it, callbacks_.end());  // calls unlinkToDeath on deleted callbacks.
    return removed ? ndk::ScopedAStatus::ok()
                   : ndk::ScopedAStatus::fromServiceSpecificError(IHealth::STATUS_NOT_FOUND);
}

// TODO(b/177269435): Merge updateInternal() to update()
ndk::ScopedAStatus Health::updateInternal() {
    HealthInfo health_info;
    auto res = getHealthInfo(&health_info);
    if (res.isOk()) {
        battery_monitor_.logValues();
        return res;
    }
    LOG(DEBUG) << "Cannot call getHealthInfo for update(): " << res.getDescription();
    // Propagate service specific errors. If there's none, report unknown error.
    if (res.getServiceSpecificError() != 0) {
        return res;
    }
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(IHealth::STATUS_UNKNOWN,
                                                                   res.getDescription().c_str());
}

ndk::ScopedAStatus Health::update() {
    if (auto res = updateInternal(); !res.isOk()) {
        return res;
    }
    HealthInfo health_info;
    if (auto res = getHealthInfo(&health_info); !res.isOk()) {
        return res;
    }
    OnHealthInfoChanged(health_info);
    return ndk::ScopedAStatus::ok();
}

void Health::OnHealthInfoChanged(const HealthInfo& health_info) {
    // Notify all callbacks
    std::unique_lock<decltype(callbacks_lock_)> lock(callbacks_lock_);
    // is_dead notifies a callback and return true if it is dead.
    auto is_dead = [&](const auto& linked) {
        auto res = linked.callback()->healthInfoChanged(health_info);
        return IsDeadObjectLogged(res);
    };
    auto it = std::remove_if(callbacks_.begin(), callbacks_.end(), is_dead);
    callbacks_.erase(it, callbacks_.end());  // calls unlinkToDeath on deleted callbacks.
    lock.unlock();

    // adjusts uevent / wakealarm periods
    HalHealthLoop::OnHealthInfoChanged(health_info);
}

void Health::BinderEvent(uint32_t /*epevents*/) {
    if (binder_fd_ >= 0) {
        ABinderProcess_handlePolledCommands();
    }
}

void Health::Init(struct healthd_config* config) {
    set_service(shared_from_this());
    // Set up epoll and get uevent / wake alarm periods
    HalHealthLoop::Init(config);

    LOG(INFO) << instance_name() << " instance initializing with healthd_config...";

    binder_status_t status = ABinderProcess_setupPolling(&binder_fd_);

    if (status == ::STATUS_OK && binder_fd_ >= 0) {
        auto binder_event = [](auto* health_loop, uint32_t epevents) {
            static_cast<Health*>(health_loop)->BinderEvent(epevents);
        };
        if (RegisterEvent(binder_fd_, binder_event, EVENT_NO_WAKEUP_FD) != 0) {
            PLOG(ERROR) << instance_name() << " instance: Register for binder events failed";
        }
    }

    std::string health_name = IHealth::descriptor + "/"s + instance_name();
    CHECK_EQ(STATUS_OK, AServiceManager_addService(this->asBinder().get(), health_name.c_str()))
            << instance_name() << ": Failed to register HAL";

    LOG(INFO) << instance_name() << ": Hal init done";
}

// Unlike hwbinder, for binder, there's no need to explicitly call flushCommands()
// in PrepareToWait(). See b/139697085.

}  // namespace aidl::android::hardware::health
