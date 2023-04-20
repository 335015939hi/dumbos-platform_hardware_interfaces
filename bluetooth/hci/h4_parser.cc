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

#include <iostream>
#include "h4_parser.h"

namespace android::hardware::bluetooth::hci {

static size_t GetHeaderSize(H4Parser::Idc idc) {
  switch (idc) {
    case H4Parser::Idc::kCommand: return 3;
    case H4Parser::Idc::kAcl: return 4;
    case H4Parser::Idc::kSco: return 3;
    case H4Parser::Idc::kEvent: return 2;
    case H4Parser::Idc::kIso: return 4;
    default:
      std::cerr << "Received invalid Idc: " << std::hex << idc << std::endl;
      std::abort();
  }
}

static size_t GetPayloadSize(H4Parser::Idc idc, uint8_t const* header) {
  switch (idc) {
    case H4Parser::Idc::kCommand: return size_t(header[2]);
    case H4Parser::Idc::kAcl: return size_t(header[2]) | (size_t(header[3]) << 8);
    case H4Parser::Idc::kSco: return size_t(header[2]);
    case H4Parser::Idc::kEvent: return size_t(header[1]);
    case H4Parser::Idc::kIso:
      return (size_t(header[2]) | size_t(header[3]) << 8) & 0x3fffU;
    default: std::abort();
  }
}

void H4Parser::Reset() {
  idc_ = Idc::kUnknown;
  state_ = State::kReadIdc;
  packet_.clear();
  bytes_requested_ = 1;
}

size_t H4Parser::Consume(std::vector<uint8_t> const& data) {
  return Consume(data.data(), data.size());
}

size_t H4Parser::Consume(uint8_t const* data, size_t data_len) {
  while (data_len > 0) {
    size_t requested_data_len = std::min(data_len, bytes_requested_);
    ConsumeRequested(data, requested_data_len);
    data += requested_data_len;
    data_len -= requested_data_len;
  }
  return bytes_requested_;
}

size_t H4Parser::ConsumeRequested(uint8_t const* data, size_t data_len) {
  switch (state_) {
    case State::kReadIdc:
      idc_ = static_cast<Idc>(data[0]);
      state_ = State::kReadHeader;
      bytes_requested_ = GetHeaderSize(idc_);
      break;

    case State::kReadHeader:
      bytes_requested_ -= data_len;
      packet_.insert(packet_.end(), data, data + data_len);

      if (bytes_requested_ == 0) {
        state_ = State::kReadPayload;
        bytes_requested_ = GetPayloadSize(idc_, packet_.data());
      }

      // Fallthrough to handle payload of empty length.
      data_len = 0;
      [[fallthrough]]

    case State::kReadPayload:
      bytes_requested_ -= data_len;
      packet_.insert(packet_.end(), data, data + data_len);

      if (bytes_requested_ == 0) {
        callback_(idc_, packet_);
        Reset();
      }

      break;
  }

  return bytes_requested_;
}

}  // namespace android::hardware::bluetooth::hci
