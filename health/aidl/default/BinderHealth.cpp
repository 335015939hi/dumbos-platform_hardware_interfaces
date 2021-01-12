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

#include <algorithm>

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <utils/Errors.h>

#include <health-impl/BinderHealth.h>
#include <health-impl/DeathRecipient.h>
#include <health-impl/LinkedCallback.h>

using std::string_literals::operator""s;

namespace aidl::android::hardware::health {

namespace {
bool IsDeadObjectLogged(const ndk::ScopedAStatus& ret) {
    if (ret.isOk()) return false;
    if (ret.getStatus() == ::STATUS_DEAD_OBJECT) return true;
    LOG(ERROR) << "Cannot call healthInfoChanged on callback: " << ret.getDescription();
    return false;
}
}  // namespace

BinderHealth::BinderHealth(const std::string& name, std::shared_ptr<IHealth> impl)
    : HalHealthLoop(name, impl), death_recipient_(std::make_unique<DeathRecipient>()) {
    CHECK_NE(this, impl.get());
    CHECK(!impl->isRemote());
}

//
// Methods that handle callbacks.
//

ndk::ScopedAStatus BinderHealth::registerCallback(
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

ndk::ScopedAStatus BinderHealth::unregisterCallback(
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

ndk::ScopedAStatus BinderHealth::update() {
    if (auto res = service()->update(); !res.isOk()) {
        return res;
    }
    HealthInfo health_info;
    if (auto res = service()->getHealthInfo(&health_info); !res.isOk()) {
        return res;
    }
    OnHealthInfoChanged(health_info);
    return ndk::ScopedAStatus::ok();
}

void BinderHealth::OnHealthInfoChanged(const HealthInfo& health_info) {
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

void BinderHealth::BinderEvent(uint32_t /*epevents*/) {
    if (binder_fd_ >= 0) {
        ABinderProcess_handlePolledCommands();
    }
}

void BinderHealth::Init(struct healthd_config* config) {
    // Set up epoll and get uevent / wake alarm periods
    HalHealthLoop::Init(config);

    LOG(INFO) << instance_name() << " instance initializing with healthd_config...";

    binder_status_t status = ABinderProcess_setupPolling(&binder_fd_);

    if (status == ::STATUS_OK && binder_fd_ >= 0) {
        auto binder_event = [](auto* health_loop, uint32_t epevents) {
            static_cast<BinderHealth*>(health_loop)->BinderEvent(epevents);
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
