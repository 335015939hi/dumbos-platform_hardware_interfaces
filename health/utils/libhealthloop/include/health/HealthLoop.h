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
#pragma once

#include <memory>
#include <vector>

#include <android-base/unique_fd.h>
#include <healthd/healthd.h>

namespace android {
namespace hardware {
namespace health {

class HealthLoop {
  public:
    using BoundFunction = std::function<void(HealthLoop*, uint32_t /* epevents */)>;

    HealthLoop();

    // Client is responsible for holding this forever. Process will exit
    // when this is destroyed.
    virtual ~HealthLoop();

    // Initialize and start the main loop. This function does not exit unless
    // the process is interrupted.
    int StartLoop();

    // Register an epoll event. When there is an event, |func| will be
    // called with |this| as the first argument and |epevents| as the second.
    int RegisterEvent(int fd, BoundFunction func, EventWakeup wakeup);

    // Helper for implementing BatteryUpdate(). An implementation of
    // BatteryUpdate should get charger_online from BatteryMonitor::update(),
    // then reset wake alarm interval by calling SetChargerOnline.
    void SetChargerOnline(bool charger_online);

  protected:
    // healthd_mode_ops overrides. Note that healthd_mode_ops->battery_update
    // is missing because it is only used by BatteryMonitor.
    virtual void Init(healthd_config* config) = 0;
    virtual void Heartbeat() = 0;
    virtual int PrepareToWait() = 0;

    // Note that this is NOT healthd_mode_ops->battery_update(BatteryProperties*),
    // which is called by BatteryMonitor after values are fetched. This is the
    // implementation of healthd_battery_update(), which calls
    // the correct IHealth::update(),
    // which calls BatteryMonitor::update(), which calls
    // healthd_mode_ops->battery_update(BatteryProperties*).
    virtual void ScheduleBatteryUpdate() = 0;

  private:
    struct EventHandler {
        HealthLoop* object = nullptr;
        int fd;
        BoundFunction func;
    };

    int InitInternal();
    void MainLoop();
    void WakeAlarmInit();
    void WakeAlarmEvent(uint32_t);
    void UeventInit();
    void UeventEvent(uint32_t);
    void WakeAlarmSetInterval(int interval);
    void PeriodicChores();

    android::base::unique_fd epollfd_;
    struct healthd_config healthd_config_;
    android::base::unique_fd wakealarm_fd_;
    android::base::unique_fd uevent_fd_;
    std::vector<std::unique_ptr<EventHandler>> event_handlers_;
    // -1 for no epoll timeout
    int awake_poll_interval_;
    int wakealarm_wake_interval_;
};

}  // namespace health
}  // namespace hardware
}  // namespace android
