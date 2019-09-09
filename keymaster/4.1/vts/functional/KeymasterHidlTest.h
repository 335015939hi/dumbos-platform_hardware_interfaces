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

#ifndef HARDWARE_INTERFACES_KEYMASTER_41_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_H_
#define HARDWARE_INTERFACES_KEYMASTER_41_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_H_

#include <android/hardware/keymaster/4.1/IKeymasterDevice.h>

#include <VtsHalHidlTargetTestBase.h>

namespace android::hardware::keymaster::V4_1::test {

class KeymasterHidlEnvironment : public ::testing::VtsHalHidlTargetTestEnvBase {
  public:
    // get the test environment singleton
    static KeymasterHidlEnvironment* Instance() {
        static KeymasterHidlEnvironment* instance = new KeymasterHidlEnvironment;
        return instance;
    }

    void registerTestServices() override { registerTestService<IKeymasterDevice>(); }

  private:
    KeymasterHidlEnvironment(){};

    GTEST_DISALLOW_COPY_AND_ASSIGN_(KeymasterHidlEnvironment);
};

}  // namespace android::hardware::keymaster::V4_1::test

#endif  // HARDWARE_INTERFACES_KEYMASTER_41_VTS_FUNCTIONAL_KEYMASTER_HIDL_TEST_H_
