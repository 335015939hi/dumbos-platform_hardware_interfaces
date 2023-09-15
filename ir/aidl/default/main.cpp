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

#include <aidl/android/hardware/ir/BnConsumerIr.h>
#include <aidl/android/hardware/ir/ConsumerIrFreqRange.h>
#include <android-base/logging.h>
#include <android/binder_interface_utils.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <numeric>

using ::aidl::android::hardware::ir::ConsumerIrFreqRange;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const ConsumerIrFreqRange consumerir_freqs[] = {
        {.minHz = 30000, .maxHz = 30000}, {.minHz = 33000, .maxHz = 33000},
        {.minHz = 36000, .maxHz = 36000}, {.minHz = 38000, .maxHz = 38000},
        {.minHz = 40000, .maxHz = 40000}, {.minHz = 56000, .maxHz = 56000},
};

namespace aidl::android::hardware::ir {

class ConsumerIr : public BnConsumerIr {
  public:
    ::ndk::ScopedAStatus getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) override;
    ::ndk::ScopedAStatus transmit(int32_t in_carrierFreqHz,
                                  const std::vector<int32_t>& in_pattern) override;
};

::ndk::ScopedAStatus ConsumerIr::getCarrierFreqs(std::vector<ConsumerIrFreqRange>* _aidl_return) {
    *_aidl_return = std::vector<ConsumerIrFreqRange>(
            consumerir_freqs, consumerir_freqs + ARRAY_SIZE(consumerir_freqs));
    return ::ndk::ScopedAStatus::ok();
}

::ndk::ScopedAStatus ConsumerIr::transmit(int32_t in_carrierFreqHz,
                                          const std::vector<int32_t>& in_pattern) {
    if (in_carrierFreqHz > 0) {
        int total_time = std::accumulate(in_pattern.begin(), in_pattern.end(), 0);
        /* simulate the time spent transmitting by sleeping */
        LOG(DEBUG) << "transmit for " << total_time << " uS at " << in_carrierFreqHz << " Hz";
        usleep(total_time);
        return ::ndk::ScopedAStatus::ok();
    } else {
        return ::ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

}  // namespace aidl::android::hardware::ir

using aidl::android::hardware::ir::ConsumerIr;

int main() {
    auto binder = ::ndk::SharedRefBase::make<ConsumerIr>();
    const std::string name = std::string() + ConsumerIr::descriptor + "/default";
    CHECK_EQ(STATUS_OK, AServiceManager_addService(binder->asBinder().get(), name.c_str()))
            << "Failed to register " << name;

    ABinderProcess_setThreadPoolMaxThreadCount(0);
    ABinderProcess_joinThreadPool();

    return EXIT_FAILURE;  // should not reached
}
