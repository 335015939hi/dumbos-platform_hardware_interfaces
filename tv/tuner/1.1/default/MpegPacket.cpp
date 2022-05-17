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

#include "MpegPacket.h"
#include <utils/Log.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {

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
    
public:
	MpegPacket(vector<uint8_t> _p) : packet(MPEG_PACKET_SIZE, 0), type(PType::UNKNOWN), ispayloaddirty(false) {
		this->packet = _p;
		header = 0x0;
        setPacketNumber();
		//read head
		for (int i = 0; i<MPEG_HEADER_SIZE; i++) {
			header <<= 8;
			header |= packet[i];
		}
		//check the packet head
		assert(packet[0] == 0x47);

		this->TEI = (header & 0x800000) >> 23;
		this->PUSI = (header & 0x400000) >> 22;
		this->TransportPriority = (header & 0x200000) >> 21;
		this->PID = (header & 0x1fff00) >> 8;
		this->TSC = (header & 0xc0) >> 6;
		this->AdaptationFieldControl = (header & 0x30) >> 4;
		this->ContinuityCounter = (header & 0xf);
        
		this->type = PType::UNKNOWN; 
        //if (TEI)
			//ALOGD("Corrupted packet");

		switch (AdaptationFieldControl) {
		case AFC_PAYONLY:
            if (PUSI) {
                this->pointer_field = packet[4];
            }
            else {
                this->pointer_field = 0x0;
            }
            
			//read payload
            this->payloadoffset = MPEG_HEADER_SIZE+PUSI+pointer_field;
			for (int i = payloadoffset; i < MPEG_PACKET_SIZE; i++)
				payload.push_back(packet[i]);
			break;
		case AFC_AFCONLY:
			this->type = PType::AFCONLY;   
            if (containsPCR())        
                this->PCR=readPCR();
			break;
		case AFC_AFCANDPAYLOAD:{          
            if (containsPCR())       
                this->PCR=readPCR();  
            
			uint8_t AFLength = packet[4];
            if (PUSI)
                this->pointer_field = packet[4 + 1 + AFLength];//behind AF field 
            else
                this->pointer_field = 0x0;  
            this->payloadoffset = MPEG_HEADER_SIZE + PUSI + AFLength + 1 + pointer_field;
			for (int i = payloadoffset ; i<MPEG_PACKET_SIZE; i++) //+1 for AF Field Length byte, +1 because we do not want pointer_field in payload
				payload.push_back(packet[i]);
			break;
        }
        default:
            //ALOGW("Unknown AdaptationFieldControl flag value");
            break;
		}
	}

	PType getPacketType() {
		return this->type;
	}

	vector<uint8_t> &getPayload() { return this->payload; }
	uint32_t getHeader()          { return this->header; }
	uint16_t getPID()             { return this->PID; }
    uint32_t getPUSI()            { return this->PUSI; }
	uint32_t getCC()              { return this->ContinuityCounter; }
    uint64_t getPacketNo()        { return this->packet_number; }
    PCR_t    getPCR()             { return this->PCR;     }
    
	vector<uint8_t>&getPacket(){ 
        if (ispayloaddirty) updatePayload(); 
        return this->packet;  
    }
    
    PCR_t readPCR(){
        unsigned offset = 6;
        uint64_t pcrbase = ((uint64_t) packet[offset])<<25;
        pcrbase |= ((uint64_t) packet[offset+1])<<17;
        pcrbase |= ((uint64_t) packet[offset+2])<<9;
        pcrbase |= ((uint64_t) packet[offset+3])<<1;
        pcrbase |= ((uint64_t) (0x80&packet[offset+4]))>>7;
        
        uint64_t pcr_extension = ((uint64_t)0x01 & packet[offset+4] )<<8;
        pcr_extension |= packet[offset+5];
        return PCR_t(pcrbase,pcr_extension);
    }
    bool containsAdaptationField(){
        return AdaptationFieldControl==AFC_AFCONLY ||
        AdaptationFieldControl==AFC_AFCANDPAYLOAD;
    }
    
    bool containsPCR(){
        return packet[5]&0x10;
    }
    
    bool isDiscontunityIndicator(){
        return packet[5]&0x80;
    }
    
    void appendToPayload(vector<uint8_t>& appendablePayload){   
        payload.insert(payload.end(), appendablePayload.begin(), appendablePayload.end());
        ispayloaddirty=true;
    }
    
    void updatePayload(){
        ispayloaddirty = false;
        //lazy update
        packet.erase(packet.begin()+payloadoffset, packet.end());
        packet.insert(packet.end(), payload.begin(), payload.end());
    }
    
    size_t size(){ return this->packet.size(); }
    
    bool operator==(MpegPacket &rhs ) {
        if (ispayloaddirty) updatePayload();
        if (packet.size() != rhs.size()) return false;
        for (int i=0; i<packet.size();i++)
            if (packet[i]!=rhs[i]) return false;
        return true;
    }
    
	uint8_t& operator[](unsigned idx) {
        if (ispayloaddirty) updatePayload();
		return this->packet[idx];
	}
    void setType(PType _type){
        this->type=_type;
    }
};


class Table{
protected:
    MpegPacket* packet;
    vector<uint8_t>payload;
    uint8_t table_id,section_number, last_section_number;
    uint16_t ts_id;
    
    Table(MpegPacket* _p){
        this->packet = _p;
        this->payload = packet->getPayload();
        this->table_id = payload[0];
          
    }
    //Iterate over descriptors, find one with right tag
    vector<uint8_t>* findFirstDescForTag(uint16_t desclen, vector<uint8_t>&payload, const unsigned offset, const uint8_t TAG){
        unsigned tagindex=0;
        while (tagindex<desclen){
            if (!(tagindex+1<desclen))
                break;
            unsigned size = payload[offset+tagindex+1];;
            if (payload[offset+tagindex]==TAG){
                return new vector<uint8_t>(payload.begin()+offset+tagindex, 
                      payload.begin()+offset+tagindex+size+2);//+2 for 2 start bytes - tag and length
                break;
            }
            tagindex+=size+2; //jump to next descriptor
        }
        return nullptr;
    }
    public:
    MpegPacket* getPacket(){
        return packet;
    }
};

class PMT : public Table {
    vector<uint16_t> componentPIDs;
    
public:
    PMT(MpegPacket* _p):Table(_p){        
        packet->setType(PType::PMT);
        
        assert (table_id == TABID_PROGRAM_MAP_SECTION);
        uint16_t section_length = (((uint16_t)payload[1]&0x0F)<<8) | payload[2];
        uint16_t program_info_length = (((uint16_t)payload[10]&0xF)<<8)| payload[11];
        
        unsigned section_end = section_length+3; // payload index must not exceed this index, no valid data beyond it, payload[section_end] is invalid
        const unsigned components_loop_i_start = 12+program_info_length;;
        unsigned components_loop_i = components_loop_i_start;
        while (components_loop_i<(section_end-4)){
            uint16_t elementaryPID = (((uint16_t)payload[(components_loop_i+1)]&0x1F)<<8)| payload[(components_loop_i+2)];
            componentPIDs.push_back(elementaryPID);
            unsigned ES_info_length = (((uint16_t)payload[(components_loop_i+3)]&0x0F)<<8)| payload[(components_loop_i+4)];
            components_loop_i+=5+ES_info_length;
        }   
    }
    
    vector<uint16_t>& getComponendPIDs(){ return componentPIDs; }
};

class Program{
    uint16_t program_number, program_map_pid;

public:
    Program(uint16_t pn, uint16_t pid):
    program_number(pn),program_map_pid(pid){};
    
    uint16_t getProgramNumber(){ return program_number; }
    uint16_t getProgramMapPID(){ return program_map_pid; }
   
}; 

class PAT : protected Table{
	uint16_t ts_id;
	vector<Program*> programs;
	map<uint16_t, PMT*> PMTs;
		
public:
	PAT(MpegPacket* _p):Table(_p){
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
	uint16_t getTsID() { return ts_id; }
	vector<Program*>& getPrograms() { return programs; }
		
	PMT* getPMT(uint16_t programnumber, vector<MpegPacket*>& packets){
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
		
	~PAT() {
		for (auto const& pmt: PMTs)
			delete pmt.second;
	}
};

}  // namespacemplementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

