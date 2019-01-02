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

#define LOG_TAG "IdentityCredentialHidlHalTest"
#include <cutils/log.h>

#include <android/hardware/identity_credential/1.0/IIdentityCredential.h>

#include <VtsHalHidlTargetCallbackBase.h>
#include <VtsHalHidlTargetTestBase.h>

namespace android {
namespace hardware {
namespace identity_credential {
namespace V1_0 {
namespace test {

TEST(IdentityCredentialSelfTest, Foo) {
    ASSERT_EQ(1, 1);
}

}  // namespace test
}  // namespace V1_0
}  // namespace identity_credential
}  // namespace hardware
}  // namespace android

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGI("Test result = %d", status);
    return status;
}
