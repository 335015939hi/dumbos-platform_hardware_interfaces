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

#include <aidl/android/hardware/boot/power/BnPowerManagement.h>

namespace aidl {
namespace android {
namespace hardware {
namespace boot {
namespace power {

class PowerManagement : public BnPowerManagement {
  public:
    PowerManagement() = default;
    ndk::ScopedAStatus setWarmResetFlag(bool* _aidl_return) override;
    ndk::ScopedAStatus clearWarmResetFlag(bool* _aidl_return) override;

  private:
    bool writeWarmResetFlag(bool is_warm_reset);
};

}  // namespace power
}  // namespace boot
}  // namespace hardware
}  // namespace android
}  // namespace aidl
