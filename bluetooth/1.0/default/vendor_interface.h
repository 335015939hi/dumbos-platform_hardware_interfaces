#pragma once

#include <hidl/HidlSupport.h>

#include "async_fd_watcher.h"
#include "bt_vendor_lib.h"
#include "hci_internals.h"

// TODO(eisenbach): namespace

using ::android::hardware::hidl_vec;
using PacketReadCallback = std::function<void(HciPacketType, const hidl_vec<uint8_t>&)>;

class VendorInterface {
 public:
  static bool Initialize(PacketReadCallback packet_read_cb);
  static void Shutdown();
  static VendorInterface* get();

  size_t Send(const uint8_t *data, size_t length);

  void OnFirmwareConfigured(uint8_t result);

  // Actually send the data.
  size_t SendPrivate(const uint8_t *data, size_t length);

 private:
  VendorInterface() { queued_data_.resize(0); }
  virtual ~VendorInterface() = default;

  bool Open(PacketReadCallback packet_read_cb);
  void Close();

  void OnDataReady(int fd);

  // Queue data from Send() until the interface is ready.
  hidl_vec<uint8_t> queued_data_;

  void *lib_handle_;
  bt_vendor_interface_t *lib_interface_;
  AsyncFdWatcher fd_watcher_;
  int uart_fd_;
  PacketReadCallback packet_read_cb_;
  bool firmware_configured_;

  // TODO(eisenbach): Separate HCI parser logic to separate module again?
  enum HciParserState {
    HCI_IDLE,
    HCI_TYPE_READY,
    HCI_PAYLOAD
  };
  HciParserState hci_parser_state_{HCI_IDLE};
  HciPacketType hci_packet_type_{HCI_PACKET_TYPE_UNKNOWN};
  hidl_vec<uint8_t> hci_packet_;
  size_t hci_packet_bytes_remaining_;
  size_t hci_packet_bytes_read_;
};
