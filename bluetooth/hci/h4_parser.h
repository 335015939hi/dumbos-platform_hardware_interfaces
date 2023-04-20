// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace android::hardware::bluetooth::hci {

/// # Description
///
/// Parses HCI frames received from an HCI transport which can be
/// usb, serial, tcp, etc. usually opened as a file descriptor on
/// unix platforms.
///
/// The transport is not part of this parser, rather the caller will inject
/// data received over the transport to this parser, to receive packets
/// in return.
///
/// # Usage
///
/// ```
/// H4Parser h4(packet_handler);
/// Transport transport();
///
/// size_t bytes_requested = 1;
/// for (;;) {
///    std::vector<uint8_t> data = transport.Read(bytes_requested);
///    bytes_requested = h4.Consume(data);
/// }
/// ```
class H4Parser {
 public:
  enum State {
    kReadIdc,
    kReadHeader,
    kReadPayload,
  };
  enum Idc {
    kUnknown = 0,
    kCommand = 1,
    kAcl = 2,
    kSco = 3,
    kEvent = 4,
    kIso = 5,
  };
  using Callback = std::function<void(Idc, std::vector<uint8_t> const&)>;

  H4Parser(Callback callback) : callback_(std::move(callback)) {}
  ~H4Parser() = default;

  /// Consumes the input data, optionally invoking the packet callback
  /// for completed packets.
  /// Returns the number of bytes requested from the next read.
  /// This information may be used for blocking reads, or disregarded entirely.
  size_t Consume(uint8_t const* data, size_t data_len);
  size_t Consume(std::vector<uint8_t> const& data);

  // Resets the parser to the initial state.
  void Reset();

 private:
  Callback callback_;
  Idc idc_{Idc::kUnknown};
  State state_{State::kReadIdc};
  std::vector<uint8_t> packet_;
  size_t bytes_requested_{1};

  // Consume the number of requested bytes or less.
  size_t ConsumeRequested(uint8_t const* data, size_t data_len);
};

}  // namespace android::hardware::bluetooth::hci
