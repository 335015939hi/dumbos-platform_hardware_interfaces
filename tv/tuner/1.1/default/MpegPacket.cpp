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

#define LOG_TAG "android.hardware.tv.tuner@1.1-MpegPacket"

#include "MpegPacket.h"
#include "Demux.h"
#include <utils/Log.h>

using namespace std;

namespace android {
namespace hardware {
namespace tv {
namespace tuner {
namespace V1_0 {
namespace implementation {


MpegPacket::MpegPacket(vector<uint8_t> _p) : packet(MPEG_PACKET_SIZE, 0), type(PType::UNKNOWN), ispayloaddirty(false) {
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

		ALOGD("MpegPacket> Packet Type %d", this->type);
        if (TEI) {
			ALOGD("Corrupted packet");
        }

		ALOGD("MpegPacket> AFC %d", AdaptationFieldControl);
		ALOGD("MpegPacket> PID %d", this->PID);
		
		switch (AdaptationFieldControl) {
		case AFC_PAYONLY:
			ALOGD("MpegPacket> PUSI %d", PUSI);
            if (PUSI) {
                this->pointer_field = packet[4];
            }
            else {
                this->pointer_field = 0x0;
            }
            
			//read payload
            this->payloadoffset = MPEG_HEADER_SIZE+PUSI+pointer_field;
			//Demux dem = new Demux();
			ALOGD("MpegPacket> payloadoffset %d", this->payloadoffset);
			for (int i = payloadoffset; i < MPEG_PACKET_SIZE; i++) {
				ALOGD("MpegPacket> packet[%d]: %d", i, packet[i]);
				payload.push_back(packet[i]);
				//dem.updateFilterOutput(0, payload);
			}
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
            ALOGW("Unknown AdaptationFieldControl flag value");
            break;
		}
	}

	MpegPacket::~MpegPacket() {}

	PType MpegPacket::getPacketType() {
		return this->type;
	}

	vector<uint8_t> & MpegPacket::getPayload() { return this->payload; }
	uint32_t MpegPacket::getHeader()          { return this->header; }
	uint16_t MpegPacket::getPID()             { return this->PID; }
    uint32_t MpegPacket::getPUSI()            { return this->PUSI; }
	uint32_t MpegPacket::getCC()              { return this->ContinuityCounter; }
    uint64_t MpegPacket::getPacketNo()        { return this->packet_number; }
    PCR_t MpegPacket::getPCR()             { return this->PCR;     }
    
	vector<uint8_t>& MpegPacket::getPacket(){ 
        if (ispayloaddirty) updatePayload(); 
        return this->packet;  
    }
    
    PCR_t MpegPacket::readPCR(){
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
    bool MpegPacket::containsAdaptationField(){
        return AdaptationFieldControl==AFC_AFCONLY ||
        AdaptationFieldControl==AFC_AFCANDPAYLOAD;
    }
    
    bool MpegPacket::containsPCR(){
        return packet[5]&0x10;
    }
    
    bool MpegPacket::isDiscontunityIndicator(){
        return packet[5]&0x80;
    }
    
    void MpegPacket::appendToPayload(vector<uint8_t>& appendablePayload){   
        payload.insert(payload.end(), appendablePayload.begin(), appendablePayload.end());
        ispayloaddirty=true;
    }
    
    void MpegPacket::updatePayload(){
        ispayloaddirty = false;
        //lazy update
        packet.erase(packet.begin()+payloadoffset, packet.end());
        packet.insert(packet.end(), payload.begin(), payload.end());
    }
    
    size_t MpegPacket::size(){ return this->packet.size(); }
    
    bool MpegPacket::operator==(MpegPacket &rhs ) {
        if (ispayloaddirty) updatePayload();
        if (packet.size() != rhs.size()) return false;
        for (int i=0; i<packet.size();i++)
            if (packet[i]!=rhs[i]) return false;
        return true;
    }
    
	uint8_t& MpegPacket::operator[](unsigned idx) {
        if (ispayloaddirty) updatePayload();
		return this->packet[idx];
	}
    void MpegPacket::setType(PType _type){
        this->type=_type;
    }

 template <class T>
 void Tables<T>::add(T* i) {
	 tables.push_back(i);
 }

 template <class T>
 vector<T*>& Tables<T>::get() {
	 return tables;
 }

 template <class T>
 size_t Tables<T>::size(){
	 return tables.size();
 }

 template <class T>
 T& Tables<T>::operator[](unsigned idx) {
	 return *(tables[idx]);
 }

 template <class T>
 const T& Tables<T>::operator[](unsigned idx) const {
	 return *(tables[idx]);
 }

template <class T>
Tables<T>::Tables() {}

 
 template <class T>
 void Tables<T>::cleanup() {
	 for (auto table : tables)
		delete table;
	 tables.clear();
 }
    
 Table::Table(MpegPacket* _p){
        this->packet = _p;
        this->payload = packet->getPayload();
        this->table_id = payload[0];
          
 }
 
 //Iterate over descriptors, find one with right tag
vector<uint8_t>* Table::findFirstDescForTag(uint16_t desclen, vector<uint8_t>&payload, const unsigned offset, const uint8_t TAG){
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
  
MpegPacket* Table::getPacket(){
        return packet;
}

PMT::PMT(MpegPacket* _p):Table(_p){
        packet->setType(PType::PMT);
        
        assert (table_id == TABID_PROGRAM_MAP_SECTION);
        uint16_t section_length = (((uint16_t)payload[1]&0x0F)<<8) | payload[2];
        uint16_t program_info_length = (((uint16_t)payload[10]&0xF)<<8)| payload[11];
        
        unsigned section_end = section_length+3; // payload index must not exceed this index, no valid data beyond it, payload[section_end] is invalid
        const unsigned components_loop_i_start = 12+program_info_length;;
        unsigned components_loop_i = components_loop_i_start;
        while (components_loop_i<(section_end-4)){
            uint16_t elementaryPID = (((uint16_t)payload[(components_loop_i+1)]&0x1F)<<8)| payload[(components_loop_i+2)];
			ALOGD("MpegPacket> PMT ES PID: %d", elementaryPID);
            componentPIDs.push_back(elementaryPID);
            unsigned ES_info_length = (((uint16_t)payload[(components_loop_i+3)]&0x0F)<<8)| payload[(components_loop_i+4)];
            components_loop_i+=5+ES_info_length;
        }   
}
    
 vector<uint16_t>& PMT::getComponendPIDs(){ return componentPIDs; }


Program::Program(uint16_t pn, uint16_t pid): program_number(pn),program_map_pid(pid){};
    
uint16_t Program::getProgramNumber(){ return program_number; }
uint16_t Program::getProgramMapPID(){ return program_map_pid; }


Service::Service(uint16_t id, string *spname, string *sname):ID(id), service_provider_name(spname),service_name(sname){
};

uint16_t Service::getServiceID(){ return ID; }
string Service::getServiceProviderName(){ return *service_provider_name; }
string Service::getServiceName(){ return *service_name; }
    
 Service::~Service() {  
  if (service_provider_name)
     delete service_provider_name;
  if (service_name)
     delete service_name;
}

SDT::SDT(MpegPacket* _p):Table(_p) {
			packet->setType(PType::SDT);
	
			assert(table_id == TABID_SDT_ACTUAL);  
			uint16_t section_length = (((uint16_t)payload[1] & 0x0F) << 8) | payload[2];
			ts_id = (((uint16_t)payload[3]) << 8) |payload[4];
			unsigned section_end = section_length + 3;
			const unsigned service_loop_i_start = 11;
			
			unsigned service_loop_i = service_loop_i_start;
			while (service_loop_i < (section_end -4)) { //-4 because CRC_32 follows
				uint16_t service_id = (((uint16_t)payload[service_loop_i]) << 8 ) |payload[service_loop_i+1];
				ALOGD("MpegPacket> SDT service ID: %d", service_id);
				uint16_t svdesclen = (((uint16_t)payload[service_loop_i + 3] & 0x0F) <<8 ) |payload[service_loop_i+4];
				const unsigned desc_i_start = service_loop_i + 5;
				vector<uint8_t>* svdesc = findFirstDescForTag(svdesclen, payload, desc_i_start, TAG_service_descriptor);
				if (svdesc){
					uint8_t provider_name_length = (*svdesc)[3];
					uint8_t service_name_length = (*svdesc)[3 + provider_name_length + 1];
					string* provider_name = new string(svdesc->begin() + 4, svdesc->begin() + provider_name_length + 4);
					ALOGD("MpegPacket> SDT provider_name %s", provider_name->c_str());
					unsigned offset_sname = provider_name_length + 5;
					string* service_name = new string(svdesc->begin()+offset_sname, svdesc->begin()+service_name_length+offset_sname);
					ALOGD("MpegPacket> SDT service name %s", service_name->c_str());
					services.push_back(new Service(service_id, provider_name, service_name));
					delete svdesc;
				}
				service_loop_i += 4 + svdesclen + 1;
			}
}
		
uint16_t SDT::getTsID() { return ts_id; }
vector<Service*>& SDT::getServices() { return services; }

}  // namespacemplementation
}  // namespace V1_0
}  // namespace tuner
}  // namespace tv
}  // namespace hardware
}  // namespace android

