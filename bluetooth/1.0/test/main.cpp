#define LOG_TAG "hidl_test"
#include <android-base/logging.h>

#include <android/hardware/bluetooth/1.0/IBluetoothHci.h>
#include <android/hardware/bluetooth/1.0/IBluetoothHciEvent.h>

#include <gtest/gtest.h>
#include <inttypes.h>
#if GTEST_IS_THREADSAFE
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#else
#error "GTest did not detect pthread library."
#endif

#include <hidl/IServiceManager.h>
#include <hidl/Status.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>

#include <utils/Condition.h>
#include <utils/Timers.h>

using ::android::hardware::bluetooth::V1_0::HciPacket;
using ::android::hardware::bluetooth::V1_0::IBluetoothHci;
using ::android::hardware::bluetooth::V1_0::IBluetoothHciEvent;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::Mutex;
using ::android::Condition;

struct Hci : public IBluetoothHci {
    virtual Return<void> sendHci(const HciPacket& packet) override;
    virtual Return<void> registerEventCb(const sp<IBluetoothHciEvent> &cb)
            override;
};

struct HciEvent : public IBluetoothHciEvent {
    virtual Return<void> sendHciEvent(const HciPacket& event) override;
};

template<typename I>
static std::string arraylikeToString(const I data, size_t size) {
    std::string out = "[";
    for (size_t i = 0; i < size; ++i) {
        if (i > 0) {
            out += ", ";
        }

        out += ::android::String8::format("%d", data[i]).string();
    }
    out += "]";

    return out;
}

static std::string vecToString(const hidl_vec<int32_t> &vec) {
    return arraylikeToString(vec, vec.size());
}

static std::string vecToString(const hidl_vec<uint8_t> &vec) {
    return arraylikeToString(vec, vec.size());
}

Return<void> Hci::sendHci(const HciPacket& command) {
    ALOGI("sendHci(%s) called", vecToString(command).c_str());
    return Void();
}

Return<void> Hci::registerEventCb(const sp<IBluetoothHciEvent> &cb) {
    ALOGI("registerEventCb called cb = %p", cb.get());
    return Void();
}

Return<void> HciEvent::sendHciEvent(const HciPacket& event) {
    ALOGI("sendHciEvent(%s) called", vecToString(event).c_str());
    return Void();
}

#define EXPECT_OK(ret) EXPECT_TRUE(ret.getStatus().isOk())

template <class T>
static void startServer(T server,
                        const char *serviceName,
                        const char *tag) {
    using namespace android::hardware;
    ALOGI("SERVER(%s) registering", tag);
    server->registerAsService(serviceName);
    ALOGI("SERVER(%s) starting", tag);
    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool(); // never ends. needs kill().
    ALOGI("SERVER(%s) ends.", tag);
}


class HidlTest : public ::testing::Test {
public:
    sp<::android::hardware::IBinder> service;
    sp<IBluetoothHci> command;
    sp<IBluetoothHciEvent> event;
    sp<::android::hardware::IBinder> cbService;
    virtual void SetUp() override {
        ALOGI("Test setup beginning...");
        using namespace android::hardware;
        command = IBluetoothHci::getService("bluetoothHci");
        CHECK(command != NULL);

        event = IBluetoothHciEvent::getService("bluetoothHciEvent");
        CHECK(event != NULL);

        ALOGI("Test setup complete");
    }
    virtual void TearDown() override {
    }
};

class HidlEnvironment : public ::testing::Environment {
private:
    pid_t bluetoothHciServerPid, bluetoothHciEventServerPid;
public:
    virtual void SetUp() {
        ALOGI("Environment setup beginning...");
        // use fork to create and kill to destroy server processes.
        if ((bluetoothHciServerPid = fork()) == 0) {
            startServer(new Hci, "bluetoothHci", "HciPacket"); // never returns
            return;
        }

        if ((bluetoothHciServerPid = fork()) == 0) {
            startServer(new HciEvent, "bluetoothHciEvent", "HciEvent"); // never returns
            return;
        }

        // Fear you not, I am parent.
        sleep(1);
        ALOGI("Environment setup complete.");
    }

    virtual void TearDown() {
        // clean up by killing server processes.
        ALOGI("Environment tear-down beginning...");
        ALOGI("Killing servers...");
        if(kill(bluetoothHciEventServerPid, SIGTERM)) {
            ALOGE("Could not kill bluetoothHciEventServer; errno = %d", errno);
        } else {
            int status;
            ALOGI("Waiting for bluetoothHciEventServer to exit...");
            waitpid(bluetoothHciEventServerPid, &status, 0);
            ALOGI("Continuing...");
        }
        if(kill(bluetoothHciServerPid, SIGTERM)) {
            ALOGE("Could not kill bluetoothHciServer; errno = %d", errno);
        } else {
            int status;
            ALOGI("Waiting for bluetoothHciServer to exit...");
            waitpid(bluetoothHciServerPid, &status, 0);
            ALOGI("Continuing...");
        }
        ALOGI("Servers all killed.");
        ALOGI("Environment tear-down complete.");
    }
};

TEST_F(HidlTest, BluetoothSimpleHciCommand) {
    HciPacket pkt;
    pkt.resize(12);
    for (size_t i = 0; i < pkt.size(); ++i) {
        pkt[i] = i + 5 ;
    }
    ALOGI("CLIENT call sendHci.");
    EXPECT_OK(command->sendHci(pkt));
    ALOGI("CLIENT sendHci returned.");
    EXPECT_EQ(true, true);
}

TEST_F(HidlTest, BluetoothSimpleRegisterHciEvent) {
    ALOGI("CLIENT call sendHciEvent.");
    nsecs_t before = systemTime();
    EXPECT_OK(command->registerEventCb(event));
    nsecs_t call_time = systemTime() - before;
    ALOGI("CLIENT callMe returned after %d ms.", static_cast<int>(call_time));
    EXPECT_EQ(true, true);
}

int main(int argc, char **argv) {

    ::testing::AddGlobalTestEnvironment(new HidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();

    ALOGI("Test result = %d", status);
    return status;
}
