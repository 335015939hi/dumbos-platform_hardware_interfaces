/*
 * Copyright 2020 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_1_MPEGPACKET_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_1_MPEGPACKET_H_

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include  <iomanip>
#include <string>
#include <stdlib.h>
#include <algorithm> 

#include <vector>
#include <map>

#include <assert.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

const uint32_t MPEG_PAYLOAD_SIZE = 184;
const uint32_t MPEG_HEADER_SIZE = 4;
const uint32_t MPEG_PACKET_SIZE = MPEG_HEADER_SIZE + MPEG_PAYLOAD_SIZE;
const uint32_t AFC_PAYONLY = 0x01;
const uint32_t AFC_AFCONLY = 0x02;
const uint32_t AFC_AFCANDPAYLOAD = 0x03;
const uint32_t SCR_NOTSCR = 0x0;
const uint8_t TABID_PROGRAM_ASSOCIATION_SECTION = 0x00;
const uint8_t TABID_PROGRAM_MAP_SECTION = 0x02;

typedef struct PCR_t{
    uint64_t base,extension;
    PCR_t(){ base = 0; extension = 0; };
    PCR_t( uint64_t _b, uint64_t _e ){
        base = _b;
        extension = _e;
    }
    uint64_t calculatePCR(){return (base*300)+extension; }
}PCR_t;

enum class PType {
	AFCONLY,
	PAT,
	PMT,
	UNKNOWN
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_1_DEMUX_H_
