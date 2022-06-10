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
const uint8_t TABID_SDT_ACTUAL = 0x42;
const uint32_t PAT_PID = 0x0000; 
const uint32_t SDT_PID = 0x0011;
const uint8_t TAG_service_descriptor = 0x48;

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
	SDT,
	UNKNOWN
};

class Demux;

class MpegPacket {
private:
	vector<uint8_t> packet;
	uint32_t header,
		TEI, //Set when a demodulator can't correct errors from FEC data; indicating the packet is corrupt.
		PUSI, //Set when a PES, PSI, or DVB-MIP packet begins immediately following the header.
		TransportPriority, //Set when the current packet has a higher priority than other packets with the same PID.
		PID, // Packet Identifier, describing the payload data.
		TSC,
		AdaptationFieldControl,
		pointer_field, //Sections may start at the beginning of the payload of a TS packet,
					   //but this is not a requirement, because the start of the first section in the payload of a TS packet is pointed to by the
					   //pointer_field
		ContinuityCounter; //Sequence number of payload packets (0x00 to 0x0F) within each stream (except PID 8191)
						   //Incremented per-PID, only when a payload flag is set.
	vector<uint8_t> payload;
	PType type;
    unsigned payloadoffset;
    bool ispayloaddirty;
    uint64_t packet_number;
    PCR_t PCR;

void setPacketNumber(){
    static uint64_t counter=0;
    packet_number=counter;
    counter++;
}

~MpegPacket();

public:
	MpegPacket(vector<uint8_t> _p);
	PType getPacketType();
	vector<uint8_t> &getPayload();
	uint32_t getHeader();
	uint16_t getPID();
	uint32_t getPUSI();
	uint32_t getCC();
	uint64_t getPacketNo();
	PCR_t    getPCR();
	vector<uint8_t>&getPacket();
	PCR_t readPCR();
	bool containsAdaptationField();
	bool containsPCR();
	bool isDiscontunityIndicator();
	void appendToPayload(vector<uint8_t>& appendablePayload);
	void updatePayload();
	size_t size();
	bool operator==(MpegPacket &rhs );
	uint8_t& operator[](unsigned idx);
	void setType(PType _type);    
};

class Table{
protected:
    MpegPacket* packet;
    vector<uint8_t>payload;
    uint8_t table_id,section_number, last_section_number;
    uint16_t ts_id;
    
    Table(MpegPacket* _p);
    //Iterate over descriptors, find one with right tag
    vector<uint8_t>* findFirstDescForTag(uint16_t desclen, vector<uint8_t>&payload, const unsigned offset, const uint8_t TAG);
	
 public:
    MpegPacket* getPacket();
};

class Program{
    uint16_t program_number, program_map_pid;

public:
    Program(uint16_t pn, uint16_t pid);
    
    uint16_t getProgramNumber();
    uint16_t getProgramMapPID();
   
}; 

class PMT : public Table {
    vector<uint16_t> componentPIDs;
    
public:
    PMT(MpegPacket* _p);
    
    vector<uint16_t>& getComponendPIDs();
};

class Service{
    uint16_t ID;
    string *service_provider_name, *service_name;

public:
    Service(uint16_t id, string *spname, string *sname);
    
    uint16_t getServiceID();
    string getServiceProviderName();
    string getServiceName();
    
    ~Service();
};


class SDT : public Table{
		uint16_t ts_id;
		vector<Service*> services;
public:
		SDT(MpegPacket* _p);
		
		uint16_t getTsID();
		vector<Service*>& getServices();
};

}  // namespace implementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_TV_TUNER_V1_1_DEMUX_H_
