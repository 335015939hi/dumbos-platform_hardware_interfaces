#include "BluetoothHci.h"

namespace android {
namespace hardware {
namespace bluetooth {
namespace V1_0 {
namespace implementation {

/* Transport-dependent code */

#define RETRY(fn) do {} while ((fn) == -1 \
     && ((errno == EINTR) || (errno == EAGAIN)));

/* Initialize the transport layer */
int BluetoothHci::initialize() {
    // userial_vendor_open();

  /*
    fd = open(BLUETOOTH_UART_DEVICE_PORT, O_RDWR);
    if (fd == -1) {
				ALOGE("%s: Unable to open %s: %d (%s)", __func__,
              BLUETOOTH_UART_DEVICE_PORT, fd, strerror(errno));
  */

    // start Event thread
    std::atomic_exchange(&thread_running, true);
    eventThread = std::thread([this]() { watchForEvents(); });
    if (!eventThread.joinable()) {
      ALOGE(LOG_TAG, "%s: Unable to start event thread", __func__);
      return -1;
    }

    return 0;
}

/* Initialize the transport layer */
int BluetoothHci::cleanUp() {
    // Stop eventThread.
    std::atomic_exchange(&thread_running, false);
    eventThread.join();

    // userial_close
}

/* Transmit a packet. */
void BluetoothHci::transmitPacket(const hidl_vec<uint8_t>& packet) {
    size_t txLen = 0;

    /* write() may not transmit the entire packet at once.  Keep trying. */
    while (txLen < packet.size()) {
        ssize_t ret;

        RETRY(ret = write(fd, &packet[txLen], packet.size() - txLen));

        switch (ret) {
            case -1:
                ALOGE(LOG_TAG, "%s: Error sending a packet: %s", __func__,
                      strerror(errno));
                return;
            case 0:
                ALOGE(LOG_TAG, "%s: Zero bytes sent!", __func__);
                return;
            default:
                txLen += ret;
            break;
        }
    }
}

void BluetoothHci::watchForEvents() {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(fd, &read_fds);
    int nfds = std::max(fd, nfds);
 
    while (thread_running == true) {
        /* Wait until there is data available to read. */
        int retval = select(nfds + 1, &read_fds, NULL, NULL, NULL);
        if (retval <= 0) {
            ALOGE(LOG_TAG, "%s: Waiting on file descriptor error = %s",
                      __func__, strerror(errno));
            continue;
        }

        /* Do not read if this thread has been stopped. */
        if (!thread_running) {
            break;
        }
    }
}

/* Read a single event from the file descriptor |fd|. */
void BluetoothHci::readEvent() {
    ssize_t rxLen;
    ssize_t rxParamLen = 0;
    uint8_t packetType, eventCode, paramLen;
    hidl_vec<uint8_t> event;

    /* Read the event header. */
    RETRY(rxLen = read(fd, &packetType, 1));
    assert(rxLen > 0);
    assert(packetType == HCI_TYPE_EVENT);

    RETRY(rxLen = read(fd, &eventCode, 1));
    assert(rxLen > 0);

    RETRY(rxLen = read(fd, &paramLen, 1));
    assert(rxLen > 0);

    /* Resize the event based on paramLen and copy in the header. */
    event.resize(ssize_t{paramLen + 3});
    event[0] = packetType;
    event[1] = eventCode;
    event[2] = paramLen;

    /* Read the event parameters into the event.  May require multiple reads.*/
    while (rxParamLen < ssize_t{paramLen}) {
        RETRY(rxLen = read(fd, &event[rxParamLen + 3], paramLen - rxParamLen));
        assert(rxLen >= 0);
        rxParamLen += rxLen;
    }

    /* Send the event. */
    eventInterface->sendHciEvent(event);
}

/* Methods from ::android::hardware::bluetooth::V1_0::IBluetoothHci follow. */

/*
 * Send an HCI packet to the controller.
 *
 * Initialize the transport if that hasn't been done.
 *
 */
Return<void> BluetoothHci::sendHci(const hidl_vec<uint8_t>& packet)  {
    if (initialized == false) {
				ALOGI("%s: Initializing the HCI transport", __func__);
        initialize();
        if (initialized == false) {
				    ALOGE("%s: Initializing the HCI transport failed!", __func__);
            return Void();
        }
    }
    
    // Transmit Packet (transmit_data())
    transmitPacket(packet);
    
    return Void();
}

Return<void> BluetoothHci::registerEventCb(const sp<IBluetoothHciEvent>& cb)  {
    eventInterface = cb;
    return Void();
}

IBluetoothHci* HIDL_FETCH_IBluetoothHci(const char* /* name */) {
    return new BluetoothHci();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace bluetooth
}  // namespace hardware
}  // namespace android
