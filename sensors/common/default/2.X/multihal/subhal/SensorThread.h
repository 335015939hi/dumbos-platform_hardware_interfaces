#ifndef MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_SENSORTHREAD_H_
#define MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_SENSORTHREAD_H_

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

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace android {
namespace hardware {
namespace sensors {
namespace V2_1 {
namespace subhal {
namespace implementation {

class AmbientLightSensor;

class SensorThread {
  public:
    explicit SensorThread(AmbientLightSensor*);
    ~SensorThread();

    void notifyAll();
    void start();
    void stop();
    std::unique_lock<std::mutex> lock();
    void join();

    bool isStopped() const;

    template <typename Predicate>
    void wait(Predicate p) {
        auto lck(lock());
        mWaitCV.wait(lck, p);
    }

  private:
    AmbientLightSensor* mSensor;
    std::atomic_bool mStopThread;
    std::condition_variable mWaitCV;
    std::mutex mRunMutex;
    std::thread mThread;
};

}  // namespace implementation
}  // namespace subhal
}  // namespace V2_1
}  // namespace sensors
}  // namespace hardware
}  // namespace android 


#endif  // MTK_GIT_5_10_HARDWARE_INTERFACES_SENSORS_COMMON_DEFAULT_2_X_MULTIHAL_SUBHAL_SENSORTHREAD_H_
