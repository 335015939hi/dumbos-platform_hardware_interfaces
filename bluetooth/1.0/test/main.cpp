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
using ::android::hardware::bluetooth::V1_0::HciPacketType;
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
    Hci() : maxPackets{10000}, sentPackets{} {
        sentPackets.resize(maxPackets);
        numPackets = 0;
    }

    Return<void> sendHci(const HciPacket& packet) override;
    Return<void> registerEventCb(const sp<IBluetoothHciEvent> &cb) override;

    const size_t maxPackets;
    size_t numPackets;
    hidl_vec<HciPacket> sentPackets;
};

struct HciEvent : public IBluetoothHciEvent {
    HciEvent() : maxEvents{10000}, sentEvents{} {
        sentEvents.resize(maxEvents);
        numEvents = 0;
    }

    Return<void> sendHciEvent(const HciPacket& event) override;

    const size_t maxEvents;
    size_t numEvents;
    hidl_vec<HciPacket> sentEvents;
};

static void fillPacketVector(hidl_vec<HciPacket> &vec, size_t packets,
                             size_t length, HciPacketType type) {
    vec.resize(packets);
    for (size_t i = 0; i < packets; i++) {
        vec[i].resize(length);
        vec[i][0] = static_cast<uint8_t>(type);
        vec[i][1] = i & 0xff; // encode the packet number
        vec[i][2] = (i >> 8) & 0xff;

        for (size_t j = 3; j < length; j++)
           vec[i][j] = static_cast<uint8_t>((i + j) % 256);
    }
}

Return<void> Hci::sendHci(const HciPacket& command) {
    sentPackets[numPackets++] = command;
    return Void();
}

Return<void> Hci::registerEventCb(const sp<IBluetoothHciEvent> &cb) {
    ALOGI("registerEventCb called cb = %p", cb.get());
    return Void();
}

Return<void> HciEvent::sendHciEvent(const HciPacket& event) {
    sentEvents[numEvents++] = event;
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
    ALOGI("CLIENT registerEventCb() returned after %d ns.", static_cast<int>(call_time));
    EXPECT_EQ(true, true);
}

TEST_F(HidlTest, BluetoothTimeTiming) {
    ALOGI("Measuring systemTime() call.");
    nsecs_t max_first = 0;
    nsecs_t min_first = 10000000000;
    nsecs_t max_second = 0;
    nsecs_t min_second = 10000000000;
    nsecs_t sum_first = 0;
    nsecs_t sum_second = 0;
    size_t samples = 1000000;
    for (size_t i = 0; i < samples; i++) {
        nsecs_t before = systemTime();
        nsecs_t returned = systemTime();
        nsecs_t after = systemTime();
        nsecs_t first = returned - before;
        nsecs_t second = after - returned;
        if (max_first < first)
            max_first = first;
        if (max_second < second)
            max_second = second;
        if (min_first > first)
            min_first = first;
        if (min_second > second)
            min_second = second;
        sum_first += first;
        sum_second += second;
    }
    ALOGI("TimeTiming (first) %d samples.", static_cast<int>(samples));
    ALOGI("  max %d min %d average %d", static_cast<int>(max_first), static_cast<int>(min_first), static_cast<int>(sum_first/samples));
    ALOGI("TimeTiming (second)");
    ALOGI("  max %d min %d average %d", static_cast<int>(max_second), static_cast<int>(min_second), static_cast<int>(sum_second/samples));
    EXPECT_EQ(true, true);
}

TEST_F(HidlTest, BluetoothTimeIndividualLargeHciCommands) {
    ALOGI("Timing individual packets.");
    nsecs_t max = 0;
    nsecs_t min = 10000000000;
    nsecs_t sum = 0;
    hidl_vec<HciPacket> to_send;
    size_t num_packets = 1000;
    size_t packet_length = 1000;
    fillPacketVector(to_send, num_packets, packet_length,
                     HciPacketType::HCI_TYPE_COMMAND);
    for (size_t i = 0; i < num_packets; i++) {
        nsecs_t before = systemTime();
        EXPECT_OK(command->sendHci(to_send[i]));
        nsecs_t delta = systemTime() - before;
        if (max < delta)
            max = delta;
        if (min > delta)
            min = delta;
        sum += delta;
    }
    ALOGI("Time %d packets of size %d.", static_cast<int>(num_packets), static_cast<int>(packet_length));
    ALOGI("  max %d min %d average %d", static_cast<int>(max), static_cast<int>(min), static_cast<int>(sum/num_packets));
    EXPECT_EQ(true, true);
}

TEST_F(HidlTest, BluetoothTimeManyLargeHciCommands) {
    hidl_vec<HciPacket> to_send;
    size_t num_packets = 1000;
    size_t packet_length = 1000;
    fillPacketVector(to_send, num_packets, packet_length,
                     HciPacketType::HCI_TYPE_COMMAND);
    ALOGI("CLIENT send %d packets.", static_cast<int>(to_send.size()));
    nsecs_t before = systemTime();
    for (size_t i = 0; i < to_send.size(); ++i) {
        EXPECT_OK(command->sendHci(to_send[i]));
    }
    nsecs_t total_time = systemTime() - before;
    ALOGI("CLIENT sent %d large packets in %d ns.",
          static_cast<int>(to_send.size()),
          static_cast<int>(total_time));
    EXPECT_EQ(true, true);
}

TEST_F(HidlTest, BluetoothTimeSmallHciCommands) {
    hidl_vec<HciPacket> to_send;
    size_t num_packets = 1000;
    size_t packet_length = 10;
    fillPacketVector(to_send, num_packets, packet_length,
                     HciPacketType::HCI_TYPE_COMMAND);
    ALOGI("CLIENT send %d packets.", static_cast<int>(to_send.size()));
    nsecs_t before = systemTime();
    for (size_t i = 0; i < to_send.size(); ++i) {
        EXPECT_OK(command->sendHci(to_send[i]));
    }
    nsecs_t total_time = systemTime() - before;
    ALOGI("CLIENT sent %d small packets in %d ns.",
          static_cast<int>(to_send.size()),
          static_cast<int>(total_time));
    EXPECT_EQ(true, true);
}

int main(int argc, char **argv) {

    ::testing::AddGlobalTestEnvironment(new HidlEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();

    ALOGI("Test result = %d", status);
    return status;
}
