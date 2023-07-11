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

#ifndef android_hardware_gnss_common_default_Utils_H_
#define android_hardware_gnss_common_default_Utils_H_

#include <aidl/android/hardware/gnss/BnGnss.h>
#include <aidl/android/hardware/gnss/BnGnssMeasurementInterface.h>
#include <condition_variable>

namespace android {
namespace hardware {
namespace gnss {
namespace common {

struct Utils {
    static aidl::android::hardware::gnss::GnssData getMockMeasurement(
            const bool enableCorrVecOutputs);

    static aidl::android::hardware::gnss::GnssLocation getMockLocation();

    static std::vector<aidl::android::hardware::gnss::IGnssCallback::GnssSvInfo>
    getMockSvInfoList();
};

struct ThreadBlocker {
    // returns false if unblocked:
    template <class R, class P>
    bool wait_for(std::chrono::duration<R, P> const& time) {
        std::unique_lock<std::mutex> lock(m);
        return !cv.wait_for(lock, time, [&] { return terminate; });
    }

    void notify() {
        std::unique_lock<std::mutex> lock(m);
        terminate = true;
        cv.notify_all();
    }

    void reset() {
        std::unique_lock<std::mutex> lock(m);
        terminate = false;
    }

  private:
    std::condition_variable cv;
    std::mutex m;
    bool terminate = false;
};

}  // namespace common
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_gnss_common_default_Utils_H_
