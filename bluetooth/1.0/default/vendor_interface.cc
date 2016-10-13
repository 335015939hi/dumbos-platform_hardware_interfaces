#include "vendor_interface.h"

#define LOG_TAG "android.hardware.bluetooth@1.0-impl"
#include <utils/Log.h>

#include <dlfcn.h>

static const char* VENDOR_LIBRARY_NAME = "libbt-vendor.so";
static const char* VENDOR_LIBRARY_SYMBOL_NAME =
    "BLUETOOTH_VENDOR_LIB_INTERFACE";

static const int INVALID_FD = -1;

static void firmware_config_cb(bt_vendor_op_result_t result);
static void sco_config_cb(bt_vendor_op_result_t result);
static void low_power_mode_cb(bt_vendor_op_result_t result);
static void sco_audiostate_cb(bt_vendor_op_result_t result);
static void* buffer_alloc_cb(int size);
static void buffer_free_cb(void* buffer);
static uint8_t transmit_cb(uint16_t opcode, void* buffer,
                           tINT_CMD_CBACK callback);
static void epilog_cb(bt_vendor_op_result_t result);
static void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op,
                            uint8_t av_handle);

static const bt_vendor_callbacks_t lib_callbacks = {
    sizeof(lib_callbacks), firmware_config_cb, sco_config_cb,
    low_power_mode_cb,     sco_audiostate_cb,  buffer_alloc_cb,
    buffer_free_cb,        transmit_cb,        epilog_cb,
    a2dp_offload_cb};

static const uint8_t HCI_RESET_COMMAND[] = {0x01, 0x03, 0x0c, 0x00};

namespace {

tINT_CMD_CBACK internal_command_cb;

VendorInterface *g_vendor_interface = nullptr;

static const size_t preamble_size_for_type[] = {
    0, HCI_COMMAND_PREAMBLE_SIZE, HCI_ACL_PREAMBLE_SIZE, HCI_SCO_PREAMBLE_SIZE,
    HCI_EVENT_PREAMBLE_SIZE};
static const size_t packet_length_offset_for_type[] = {
    0, HCI_LENGTH_OFFSET_CMD, HCI_LENGTH_OFFSET_ACL, HCI_LENGTH_OFFSET_SCO,
    HCI_LENGTH_OFFSET_EVT};

size_t HciGetPacketLengthForType(HciPacketType type,
                                 const hidl_vec<uint8_t>& packet) {
  size_t offset = packet_length_offset_for_type[type];
  if (type == HCI_PACKET_TYPE_ACL_DATA) {
    return (((packet[offset + 1]) << 8) | packet[offset]);
  }
  return packet[offset];
}

HC_BT_HDR* WrapPacketAndCopy(uint16_t event, const hidl_vec<uint8_t>& data) {
  size_t packet_size = data.size() + sizeof(HC_BT_HDR);
  HC_BT_HDR* packet = reinterpret_cast<HC_BT_HDR*>(new uint8_t[packet_size]);
  packet->offset = 0;
  packet->len = data.size();
  packet->layer_specific = 0;
  packet->event = event;
  // TODO(eisenbach): Avoid copy here; if BT_HDR->data can be enusred to
  // be the only way the data is accessed, a pointer could be passed here...
  memcpy(packet->data, data.data(), data.size());
  return packet;
}

}  // namespace

bool VendorInterface::Initialize(PacketReadCallback packet_read_cb) {
  assert(!g_vendor_interface);
  g_vendor_interface = new VendorInterface();
  return g_vendor_interface->Open(packet_read_cb);
}

void VendorInterface::Shutdown() {
  assert(g_vendor_interface);
  g_vendor_interface->Close();
  delete g_vendor_interface;
  g_vendor_interface = nullptr;
}

VendorInterface* VendorInterface::get() {
  return g_vendor_interface;
}

bool VendorInterface::Open(PacketReadCallback packet_read_cb) {
  // TODO(eisenbach): P0 - get local BD address somehow
  uint8_t local_bda[] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  firmware_configured_ = false;
  packet_read_cb_ = packet_read_cb;

  // Initialize vendor interface

  lib_handle_ = dlopen(VENDOR_LIBRARY_NAME, RTLD_NOW);
  if (!lib_handle_) {
    ALOGE("%s unable to open %s (%s)", __func__, VENDOR_LIBRARY_NAME,
          dlerror());
    return false;
  }

  lib_interface_ = reinterpret_cast<bt_vendor_interface_t*>(
      dlsym(lib_handle_, VENDOR_LIBRARY_SYMBOL_NAME));
  if (!lib_interface_) {
    ALOGE("%s unable to find symbol %s in %s (%s)", __func__,
          VENDOR_LIBRARY_SYMBOL_NAME, VENDOR_LIBRARY_NAME, dlerror());
    return false;
  }

  int status = lib_interface_->init(&lib_callbacks, (unsigned char*)local_bda);
  if (status) {
    ALOGE("%s unable to initialize vendor library: %d", __func__, status);
    return false;
  }

  ALOGD("%s vendor library loaded", __func__);

  // Power cycle chip

  int power_state = BT_VND_PWR_OFF;
  lib_interface_->op(BT_VND_OP_POWER_CTRL, &power_state);
  power_state = BT_VND_PWR_ON;
  lib_interface_->op(BT_VND_OP_POWER_CTRL, &power_state);

  // Get the UART socket

  int fd_list[CH_MAX] = {0};
  int fd_count = lib_interface_->op(BT_VND_OP_USERIAL_OPEN, &fd_list);

  if (fd_count != 1) {
    ALOGE("%s fd count %d != 1; we can't handle this currently...", __func__,
          fd_count);
    return false;
  }

  uart_fd_ = fd_list[0];
  if (uart_fd_ == INVALID_FD) {
    ALOGE("%s unable to determine UART fd", __func__);
    return false;
  }

  ALOGD("%s UART fd: %d", __func__, uart_fd_);

  fd_watcher_.WatchFdForNonBlockingReads(uart_fd_,
                                         [this](int fd) { OnDataReady(fd); });

  // Start configuring the firmware

  lib_interface_->op(BT_VND_OP_FW_CFG, nullptr);

  return true;
}

void VendorInterface::Close() {
  fd_watcher_.StopWatchingFileDescriptor();
  if (lib_handle_ != nullptr) dlclose(lib_handle_);
  lib_handle_ = nullptr;
}

size_t VendorInterface::Send(const uint8_t* data, size_t length) {
  if (uart_fd_ == INVALID_FD) return 0;

  size_t transmitted_length = 0;
  while (length > 0) {
    ssize_t ret =
        TEMP_FAILURE_RETRY(write(uart_fd_, data + transmitted_length, length));

    if (ret == -1) {
      if (errno == EAGAIN) continue;
      ALOGE("%s error writing to UART (%s)", __func__, strerror(errno));
      break;

    } else if (ret == 0) {
      // Nothing written :(
      ALOGE("%s zero bytes written - something went wrong...", __func__);
      break;
    }

    transmitted_length += ret;
    length -= ret;
  }

  return transmitted_length;
}

void VendorInterface::OnFirmwareConfigured(uint8_t result) {
  firmware_configured_ = true;
  // TODO(eisenbach): Don't do this blindly; cache bytes, then send...
  Send(HCI_RESET_COMMAND, 4);
}

void VendorInterface::OnDataReady(int fd) {
  switch (hci_parser_state_) {
    case HCI_IDLE: {
      uint8_t buffer[1] = {0};
      size_t bytes_read = TEMP_FAILURE_RETRY(read(fd, buffer, 1));
      assert(bytes_read == 1);
      hci_packet_type_ = static_cast<HciPacketType>(buffer[0]);
      hci_parser_state_ = HCI_TYPE_READY;
      // TODO(eisenbach): Assert here on invalid packet type and check for
      // workaround(s)
      break;
    }

    case HCI_TYPE_READY: {
      hci_packet_ = new hidl_vec<uint8_t>;
      // TODO(eisenbach): add initial size to hidl_vec constructor
      hci_packet_->resize(HCI_PREAMBLE_SIZE_MAX);
      size_t bytes_read = TEMP_FAILURE_RETRY(read(
          fd, hci_packet_->data(), preamble_size_for_type[hci_packet_type_]));
      assert(bytes_read == preamble_size_for_type[hci_packet_type_]);
      size_t packet_length =
          HciGetPacketLengthForType(hci_packet_type_, *hci_packet_);
      hci_packet_->resize(preamble_size_for_type[hci_packet_type_] +
                          packet_length);
      hci_packet_bytes_remaining_ = packet_length;
      hci_parser_state_ = HCI_PAYLOAD;
      break;
    }

    case HCI_PAYLOAD: {
      size_t bytes_read = TEMP_FAILURE_RETRY(read(
          fd, hci_packet_->data() + preamble_size_for_type[hci_packet_type_],
          hci_packet_bytes_remaining_));
      hci_packet_bytes_remaining_ -= bytes_read;
      if (hci_packet_bytes_remaining_ == 0) {
        if (firmware_configured_) {
          if (packet_read_cb_ != nullptr) {
            packet_read_cb_(hci_packet_type_, *hci_packet_);
          }
        } else {
          if (internal_command_cb != nullptr) {
            HC_BT_HDR* bt_hdr =
                WrapPacketAndCopy(HCI_PACKET_TYPE_EVENT, *hci_packet_);
            internal_command_cb(bt_hdr);
          }
        }
        delete hci_packet_;
        hci_packet_ = nullptr;
        hci_parser_state_ = HCI_IDLE;
      }
      break;
    }
  }
}

static uint8_t transmit_cb(uint16_t opcode, void* buffer,
                           tINT_CMD_CBACK callback) {
  ALOGD("%s opcode: 0x%04x, ptr: %p", __func__, opcode, buffer);
  HC_BT_HDR* bt_hdr = reinterpret_cast<HC_BT_HDR*>(buffer);

  internal_command_cb = callback;
  uint8_t type = HCI_PACKET_TYPE_COMMAND;
  VendorInterface::get()->Send(&type, 1);
  VendorInterface::get()->Send(bt_hdr->data, bt_hdr->len);
  return true;
}

static void firmware_config_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
  VendorInterface::get()->OnFirmwareConfigured(result);
}

static void sco_config_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

static void low_power_mode_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

static void sco_audiostate_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

static void* buffer_alloc_cb(int size) {
  void* p = new uint8_t[size];
  ALOGD("%s pts: %p, size: %d", __func__, p, size);
  return p;
}

static void buffer_free_cb(void* buffer) {
  ALOGD("%s ptr: %p", __func__, buffer);
  delete [] reinterpret_cast<uint8_t*>(buffer);
}

static void epilog_cb(bt_vendor_op_result_t result) {
  ALOGD("%s result: %d", __func__, result);
}

static void a2dp_offload_cb(bt_vendor_op_result_t result, bt_vendor_opcode_t op,
                            uint8_t av_handle) {
  ALOGD("%s result: %d, op: %d, handle: %d", __func__, result, op, av_handle);
}
