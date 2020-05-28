
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
#include <android-base/logging.h>
#include <android/hardware/common/MQDescriptor.h>
#include <android/hardware/common/test/BnMqTest.h>
#include <binder/Binder.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <gtest/gtest.h>
#include <sys/prctl.h>

using android::IBinder;
using android::IPCThreadState;
using android::OK;
using android::ProcessState;
using android::sp;
using android::String16;
using android::waitForService;
using android::binder::Status;
static const String16 kServiceName = String16("IMqTest");

namespace android {
namespace hardware {
namespace common {
namespace test {

class MyFmqTest : public BnMqTest {
  public:
    Status block() override {
        // pick your poison
        std::mutex m;
        m.lock();
        return Status::ok();
    }
    Status doNothing(const std::string& wasteSpace) override {
        (void)wasteSpace;
        return Status::ok();
    }

    Status getDescriptor() { return Status::ok(); }

    MQDescriptor mDescriptor;
};

TEST(BinderBufferFilled, sendTest) {
    sp<IMqTest> server = waitForService<IMqTest>(kServiceName);
    ASSERT_NE(nullptr, server);
    ASSERT_TRUE(server->block().isOk());
    Status s = server->doNothing("Hello world");
    ASSERT_TRUE(s.isOk()) << s.toString8();
}

}  // namespace test
}  // namespace common
}  // namespace hardware
}  // namespace android

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    if (fork() == 0) {
        // child process
        prctl(PR_SET_PDEATHSIG, SIGHUP);
        sp<IBinder> server = new android::hardware::common::test::MyFmqTest;
        android::defaultServiceManager()->addService(kServiceName, server);
        IPCThreadState::self()->joinThreadPool(true);
        exit(1);  // should not reach
    }
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
