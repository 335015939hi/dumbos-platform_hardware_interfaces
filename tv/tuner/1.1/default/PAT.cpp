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

//#define LOG_TAG "android.hardware.tv.tuner@1.1-MpegPacket"

#include "PAT.h"
#include <utils/Log.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

PAT::PAT(MpegPacket* _p):Table(_p) {
		packet->setType(PType::PAT);
			
		assert(table_id == TABID_PROGRAM_ASSOCIATION_SECTION); 
		uint16_t section_length = (((uint16_t)payload[1]&0x0F)<<8) | payload[2];
		ts_id=(((uint16_t)payload[3])<<8)|payload[4];
		unsigned section_end = section_length+3; // payload index must not exceed this index, no valid data beyond it, payload[section_end] is invalid
		const unsigned program_loop_i_start = 8;
		unsigned program_loop_i = program_loop_i_start;
		while (program_loop_i<(section_end-4)){
			uint16_t program_number = (((uint16_t)payload[program_loop_i])<<8) | payload[program_loop_i+1];
			if (program_number){
				uint16_t program_map_pid = (((uint16_t)payload[program_loop_i+2]&0x1F)<<8) | payload[program_loop_i+3];
				programs.push_back(new Program(program_number, program_map_pid));
			}
				program_loop_i+=3+1; //3 is length of body
		}
}

uint16_t PAT::getTsID() { return ts_id; }
vector<Program*>& PAT::getPrograms() { return programs; }
		
PMT* PAT::getPMT(uint16_t programnumber, vector<MpegPacket*>& packets){
		for (auto program: programs){
			if (program->getProgramNumber() == programnumber){
				if (PMTs.find(programnumber) != PMTs.end())
					return PMTs[programnumber];
				else{
					for (auto packet: packets)
						if (packet->getPID()==program->getProgramMapPID())
							return new PMT(packet);
					}
					break;
			}
		}
		return nullptr;
}

PAT::~PAT() {
			for (auto const& pmt: PMTs)
				delete pmt.second;
}

}  // namespacemplementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namesp

