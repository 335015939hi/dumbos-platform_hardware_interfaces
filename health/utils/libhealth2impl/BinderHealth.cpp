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

#include <health2impl/BinderHealthLoop.h>

#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <hwbinder/IPCThreadState.h>

#include <health2impl/Health.h>
#include <health2impl/Callback.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::handleTransportPoll;
using android::hardware::IPCThreadState;
using android::hardware::setupTransportPolling;

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

void BinderHealth::onFirstRef() {
    set_service(this);
}
//
// Methods that handle callbacks.
//

Return<Result> BinderHealth::registerCallback(const sp<IHealthInfoCallback_2_0>& callback) {
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
            auto it = std::find_if(callbacks_.begin(), callbacks_.end(),
                                   [wrapped](const auto& cb) { return cb.get() == wrapped; });
            if (it != callbacks_.end()) {
                callbacks_.erase(it);
            }
            // unlock
        }
    });

    return Result::SUCCESS;
}

bool BinderHealth::unregisterCallbackInternal(const sp<IBase>& callback) {
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

// The passthrough service() has |this| as a registered callback. Calling
// update() here will call this->healthInfoChanged() to be called on a background
// thread (because )
Return<Result> BinderHealth::update() {
    return service()->update();
}

Return<Result> BinderHealth::unregisterCallback(const sp<IHealthInfoCallback_2_0>& callback) {
    return unregisterCallbackInternal(callback) ? Result::SUCCESS : Result::NOT_FOUND;
}

void BinderHealth::serviceDied(uint64_t /* cookie */, const wp<IBase>& who) {
    (void)unregisterCallbackInternal(who.promote());
}

void BinderHealth::BinderEvent(uint32_t /*epevents*/) {
    if (binder_fd_ >= 0) {
        handleTransportPoll(binder_fd_);
    }
}

void BinderHealth::Init(struct healthd_config* config) {
    LOG(INFO) << instance_name() << " instance initializing with healthd_config...";

    binder_fd_.reset(setupTransportPolling());

    if (binder_fd_ >= 0) {
        auto binder_event = [](auto* health_loop, uint32_t epevents) {
            static_cast<BinderHealth*>(health_loop)->BinderEvent(epevents);
        };
        if (!RegisterEvent(binder_fd_, binder_event, EVENT_NO_WAKEUP_FD)) {
            PLOG(ERROR) << instance_name() << " instance: Register for binder events failed";
        }
    }

    // Get uevent / wake alarm periods
    HalHealthLoop::Init(config);

    service()->registerCallback(this);
}

Return<void> BinderHealth::healthInfoChanged(const V2_0::HealthInfo& health_info) {
    {
        // Notify all callbacks
        std::lock_guard<decltype(callbacks_lock_)> lock(callbacks_lock_);
        for (auto it = callbacks_.begin(); it != callbacks_.end();) {
            auto ret = (*it)->Notify(health_info);
            if (IsDeadObjectLogged(ret)) {
                it = callbacks_.erase(it);
            } else {
                ++it;
            }
        }
        // unlock
    }

    const V1_0::HealthInfo& props = health_info.legacy;
    bool charger_online = props.chargerAcOnline | props.chargerUsbOnline |
            props.chargerWirelessOnline;
    SetChargerOnline(charger_online);
}

int BinderHealth::PrepareToWait(void) {
    IPCThreadState::self()->flushCommands();
    return HalHealthLoop::PrepareToWait();
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace health
}  // namespace hardware
}  // namespace android
