
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
#include <android/hardware/common/test/BnMqTest.h>
#include <binder/Binder.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <fmq/MessageQueue.h>
#include <gtest/gtest.h>
#include <sys/prctl.h>

using android::IBinder;
using android::IPCThreadState;
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
int kNumElementsInQueue = 12;

typedef const android::aidl::MQDescriptorSync<int32_t> testAidlQueue;

class MyFmqTest : public BnMqTest {
  public:
    MyFmqTest() {}

    ::android::binder::Status createDescriptor() { return ::android::binder::Status::ok(); }

    ::android::binder::Status getDescriptor(
            ::android::aidl::MQDescriptorSync<int32_t>* _aidl_return) {
        mMq = std::make_unique<MessageQueue<int32_t, kSynchronizedReadWrite>>(
                kNumElementsInQueue, true, true /*isAidl*/);
        int payload = 12;
        mMq->write(&payload);
        *_aidl_return = *mMq->getAidlDesc();
        if (_aidl_return->isHandleValid()) {
            return ::android::binder::Status::ok();
        } else {
            return ::android::binder::Status::fromExceptionCode(Status::EX_BAD_PARCELABLE);
        }
    }

    std::unique_ptr<android::aidl::MQDescriptorSync<int32_t>> mMqDesc;
    std::unique_ptr<MessageQueue<int, kSynchronizedReadWrite>> mMq;
};

TEST(MQDescriptorTest, mqDescriptorGet) {
    // Get interface
    sp<IMqTest> server = waitForService<IMqTest>(kServiceName);
    ASSERT_NE(nullptr, server);
    ::android::aidl::MQDescriptorSync<int32_t> descriptor;
    // Get the descriptor
    ::android::binder::Status s = server->getDescriptor(&descriptor);
    ASSERT_TRUE(s.isOk()) << s.toString8();
    // TODO Need to finish the parceling for this next assertion to pass.
    ASSERT_TRUE(descriptor.isHandleValid());
    std::unique_ptr<MessageQueue<int, kSynchronizedReadWrite>> mq =
            std::make_unique<MessageQueue<int, kSynchronizedReadWrite>>(descriptor);
    EXPECT_NE(mq.get(), nullptr);
    ASSERT_TRUE(mq->getAidlDesc()->isHandleValid());
    int recieve = 0;
    EXPECT_EQ(mq->read(&recieve), true);
    EXPECT_EQ(recieve, 12);
}

TEST(MQDescriptorTest, MQWriteRead) {
    // Create message queue for AIDL(isAidl=true)
    std::unique_ptr<MessageQueue<int, kSynchronizedReadWrite>> mq =
            std::make_unique<MessageQueue<int, kSynchronizedReadWrite>>(kNumElementsInQueue, true,
                                                                        true /*isAidl*/);
    // Get the AIDL descriptor
    const android::aidl::MQDescriptorSync<int32_t>* descriptor = mq->getAidlDesc();
    ASSERT_TRUE(descriptor->isHandleValid());
    // Create a new message queue with the obtained descriptor
    std::unique_ptr<MessageQueue<int, kSynchronizedReadWrite>> mq2 =
            std::make_unique<MessageQueue<int, kSynchronizedReadWrite>>(*descriptor);
    // Validate message queue handle
    const android::aidl::MQDescriptorSync<int32_t>* descriptor2 = mq2->getAidlDesc();
    ASSERT_TRUE(descriptor2->isHandleValid());
    // Have two sides of the MQ, use mq to write and mq2 to read
    int payload = 12;
    EXPECT_EQ(mq->write(&payload), true);
    EXPECT_EQ(sizeof(*descriptor), 28);

    int recieve = 0;
    EXPECT_EQ(mq2->read(&recieve), true);
    EXPECT_EQ(recieve, 12);
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
        sp<android::hardware::common::test::MyFmqTest> server =
                new android::hardware::common::test::MyFmqTest;
        // std::unique_ptr<android::hardware::MessageQueue<int,
        // android::hardware::kSynchronizedReadWrite>> mq =
        // std::make_unique<android::hardware::MessageQueue<int,
        // android::hardware::kSynchronizedReadWrite>>
        // (android::hardware::common::test::kNumElementsInQueue, true, true/*isAidl*/);
        // server->mMqDesc = std::unique_ptr<andMQDescriptor<int,
        // android::hardware::kSynchronizedReadWrite>>(mq->getAidlDesc());
        android::defaultServiceManager()->addService(kServiceName, server);
        IPCThreadState::self()->joinThreadPool(true);
        exit(1);  // should not reach
    }
    ProcessState::self()->setThreadPoolMaxThreadCount(1);
    ProcessState::self()->startThreadPool();
    return RUN_ALL_TESTS();
}
