#define LOG_TAG "danielnorman_lazy"

#include <android/hardware/tests/lazy/1.0/ILazy.h>

#include <binder/IPCThreadState.h>
#include <gtest/gtest.h>
#include <utils/Log.h>
#include <utils/StrongPointer.h>

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::tests::lazy::V1_0::ILazy;

struct LazyTest : public ::testing::Test {};

TEST_F(LazyTest, DanielTest) {
    {
        ::android::sp<ILazy> lazy = ILazy::getService();
        EXPECT_NE(lazy, nullptr);
        EXPECT_TRUE(lazy->sayHello([&](const hidl_string message) {
                            EXPECT_STREQ(message.c_str(), "Hello, world!");
                        })
                            .isOk());
    }
    ::android::IPCThreadState::self()->flushCommands();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    return status;
}
