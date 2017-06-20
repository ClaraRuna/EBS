//include-guard
#pragma once
#ifndef DCP_H
#define DCP_H

#include <iostream>
#include <vector>
#include <iomanip>

enum device_role{
	ioDevice = 0x01,
	ioController = 0x02,
	ioMultidevice = 0x04,
	ioSupervisor = 0x08, 
	nichtGesetzt = 0xF0
};

std::ostream& operator << (std::ostream& os, const device_role& role);

struct option{
	int opt;
	int subopt;
} OPT;

typedef struct ip_param{
	u_char ip [4];
	u_char subnet[4];
	u_char gateway[4];
	
	ip_param(){
	for (int i = 0 ; i<4 ; i++){
		ip[i]=0;
		subnet[i]=0;
		gateway[i]=0;
	}
	}
}IPPAR;

typedef struct device {
	u_char MAC[6];
	std::string name;
	std::string vendor;
	u_char vendor_id [2];
	u_char device_id [2];
	device_role devRole;
	std::vector<option*> options;
	ip_param ipParam;
	
	//konstruktor
	device(){
	for (int i = 0; i<6; ++i){
		MAC[i]=0;
	}
	for (int i = 0; i<2; ++i){
		vendor_id[i]='\0';
		device_id[i]='\0';
	}
	devRole=static_cast<device_role>(0xF0);
	}
}DEV;

typedef struct dcp_data_header{
	u_char opt;
	u_char subopt;
	unsigned short length;
}DCPDH;

std::ostream& operator << (std::ostream& os, const dcp_data_header& header);

typedef struct dcpHeader {
	unsigned char serviceID;
	unsigned char serviceType;
	unsigned char xID[4];
	unsigned char respDelayFactor[2];
	unsigned char dcpDataLength[2];

	//Constructor
	dcpHeader(	unsigned char serviceID,
				unsigned char serviceType,
				unsigned char xID[4],
				unsigned char respDelayFactor[2],
				unsigned char dcpDataLength[2]) {
		this->serviceID = serviceID;
		this->serviceType = serviceType;
		for(int i=0; i<4; i++) {
			this->xID[i] = xID[i];
			if(i<2) {
				this->respDelayFactor[i] = respDelayFactor[i];
				this->dcpDataLength[i] = dcpDataLength[i];		
			}
		}
	}
}dcpHeader;

typedef struct dcpDataHeader {
	unsigned char option;
	unsigned char subOption;
	unsigned char length[2];
	unsigned char status[2];

	//Constructor
	dcpDataHeader(	unsigned char option,
					unsigned char subOption,
					unsigned char length[2],
					unsigned char status[2]) {
		this->option = option;
		this->subOption = subOption;
		for(int i=0; i<2; i++) {
			this->length[i] = length[i];
			this->status[i] = status[i];
		}
	}
}dcpDataHeader;

std::ostream& operator << (std::ostream& os, unsigned char* buffer);
std::ostream& operator << (std::ostream& os, const std::vector<option*>& o);
std::ostream& operator << (std::ostream& os, const device& dev);

#endif