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

#ifndef ANDROID_HARDWARE_TV_TUNER_V1_1_PAT_H_
#define ANDROID_HARDWARE_TV_TUNER_V1_1_PAT_H_

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
#include <MpegPacket.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

class PAT : protected Table{
		uint16_t ts_id;
		vector<Program*> programs;
		map<uint16_t, PMT*> PMTs;
			
	public:
		PAT(MpegPacket* _p);
		uint16_t getTsID();
		vector<Program*>& getPrograms();
			
		PMT* getPMT(uint16_t programnumber, vector<MpegPacket*>& packets);
			
		~PAT();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_1_PAT_H_


