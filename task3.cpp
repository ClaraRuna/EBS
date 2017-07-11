/* 
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <string.h>
#include <netinet/ether.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>

#include "header.h"
#include "outstream.h"

#define RPC_HEADER_LENGTH 80
#define NRD_DATA_LENGTH 20
#define IOD_HEADER_LENGTH 64
#define IOD_CONNECT_REQ_LENGTH 58

std::vector <struct device*> device_list; //List of devices, known from Ident-Request

unsigned char multicast_mac[6] = {0x01, 0x0E, 0xCF, 0x00, 0x00, 0x00};

unsigned char etherType_profiNET[2] = {0x88, 0x92};

unsigned char frameID_helloRequest[2] = {0xFE, 0xFC};
unsigned char frameID_getReq[2] = {0xFE, 0xFD};
unsigned char frameID_setReq[2] = {0xFE, 0xFD};
unsigned char frameID_identRequest[2] = {0xFE, 0xFE};
//0xC000 - 0xF7FF RT CLASS UDP unicast

unsigned char serviceID_getReq = 0x03;
unsigned char serviceID_setReq = 0x04;
unsigned char serviceID_identReq = 0x05;
unsigned char serviceID_helloReq = 0x06;
//rest: not in use

unsigned char serviceType_Req = 0x00;
//others for response

unsigned char xID[4] = {0x42, 0x42, 0x42, 0x42}; //for identification - dynamic?

unsigned char respDelayFactor_answer[2] = {0x00, 0x00};
unsigned char respDelayFactor_withoutImpact[2] = {0x00, 0x01};
//0x0002 - 0x1900 → mit Aufschlag

unsigned char dataLength[2] = {0x00, 0x04};
unsigned char dataLength_getReq[2] = {0x00, 0x02};
unsigned char dataLength_setIP[2] = {0x00, 0x12};

unsigned char status[2] = {0x00, 0x00};
unsigned char length[2] = {0x00, 0x00};
unsigned char lengthIP[2] = {0x00, 0x0E};

//RPC
unsigned char maxBodyLengthRPC[4] = {0x00, 0x00, 0x01, 0xd0};

int etherNameLength = 0;

int main(int argc, char *argv[]) {

	//read if from terminal
	if (argc > 1) {
		setIfName(argv);
		setEtherName(argv);
		etherNameLength = sizeof(*argv) - 1;
	} else {
		setIfName(NULL);
		setEtherName(NULL);
		etherNameLength = 15;
	}

	std::cout << "----------ProfiNET-Tool----------\n" << std::endl;

	//for reciefing Frames
	createRECSocket();
	//for sending DCP-Frames
	createRAWSocket();
	//for sending UDP-Frames
	createUDPsocket();

	std::thread first (recieveResponse, &device_list);

	usleep(1000000); //sleep 1s (in us) --> for starting thread

	//first do an Identification-Resquest
	std::cout << "-----> send Identification-Request:" << std::endl;
	struct dcpHeader dcpHeader_identReq(serviceID_identReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength);
	struct dcpDataHeader dcpDataHeader_identReq(0xFF, 0xFF, length, status);
	sendDCPFrame(multicast_mac, etherType_profiNET, frameID_identRequest, dcpHeader_identReq, dcpDataHeader_identReq, 0x00);

	usleep(1000000); //sleep 1s (in us) --> wait for answers

	std::cout << "" << std::endl; //new line
	std::cout << "Identification of " << device_list.size() << " Devices:" << std::endl;
	std::cout << "" << std::endl; //new line
	for(int i = 0; i < device_list.size(); i++) {
		std::cout << *device_list[i];
	}
	std::cout << std::endl;

	int decision; //for maintask
	while(1) {
		decision = 0;
		usleep(1000000);

		std::cout << "What you want to do next?" << std::endl;
		std::cout << "1. Hello-Request? - Press 1" << std::endl;
		std::cout << "2. Ident-Request? - Press 2" << std::endl;
		std::cout << "3. Get-Request?   - Press 3" << std::endl;
		std::cout << "4. Set-Request?   - Press 4" << std::endl;
		std::cout << "5. RPC - Read     - Press 5" << std::endl;
		std::cout << "6. RPC - Write    - Press 6\n" << std::endl;

		do {
			std::cin >> decision;
			std::cin.clear();
			std::cin.ignore();
		} while (decision < 1 || decision > 61);

		if(decision == 1) {
			std::cout << "-----> Hello-Request:\n" << std::endl;
			struct dcpHeader dcpHeader_helloReq(serviceID_helloReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength);
			struct dcpDataHeader dcpDataHeader_helloReq(0xFF, 0xFF, length, status);
			sendDCPFrame(multicast_mac, etherType_profiNET, frameID_helloRequest, dcpHeader_helloReq, dcpDataHeader_helloReq, 0x00);
			usleep(500000);
			std::cout << "Hello!\n" << std::endl;
		} else if(decision == 2) {
			std::cout << "-----> Identification-Request:\n" << std::endl;
			sendDCPFrame(multicast_mac, etherType_profiNET, frameID_identRequest, dcpHeader_identReq, dcpDataHeader_identReq, 0x00);
			usleep(1000000);
			for(int i = 0; i < device_list.size(); i++) {
				std::cout << *device_list[i];
			}
			std::cout << std::endl;
		} else if(decision == 3) {
			std::cout << "-----> Get-Request:\n" << std::endl;
			int device, param;
			std::cout << "choose a device: (from which device you want to read?)\n" << std::endl;
			std::cout << "   Name:	MAC:" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ". " << dev.name << ",	" << dev.MAC;
			}
			std::cout << "" << std::endl; //new Line
			
			do {
				std::cin >> device;
				std::cin.clear();
				std::cin.ignore();	
			} while (device < 1 || device > device_list.size());
			
			std::cout << "-----> Read from " << device_list[device-1]->name << ":" << std::endl;

			unsigned char mac[6] = {device_list[device-1]->MAC[0], device_list[device-1]->MAC[1], device_list[device-1]->MAC[2], device_list[device-1]->MAC[3], device_list[device-1]->MAC[4], device_list[device-1]->MAC[5]};

			std::cout << "\nchoose option and subotion: (which parameter you want to read?)\n" << std::endl;
			
			std::cout << device_list[device-1]->options << std::endl;
			
			do {
				std::cin >> param;
				std::cin.clear();
				std::cin.ignore();
			} while (param < 1 || param > device_list[device-1]->options.size());	//TODO: Was wenn keine Options?

			unsigned char option = device_list[device - 1]->options[param - 1]->opt;
			unsigned char subOption = device_list[device - 1]->options[param -1]->subopt;

			//always equal in Get-Request
			struct dcpHeader dcpHeader_getReq(serviceID_getReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength_getReq);
			struct dcpDataHeader dcpDataHeader_getReq(option, subOption, length, status);
			sendDCPFrame(mac, etherType_profiNET, frameID_getReq, dcpHeader_getReq, dcpDataHeader_getReq, 0x00);	
			
			//erneute Ausgabe des devices (falls bspw IP abgefragt)
			std::cout << "updated device info: " << std::endl << *device_list[device-1] << "\n" << std::endl;	
		
		} else if(decision == 4) {
			std::cout << "-----> Set-Request:\n" << std::endl;
			int device, option;
			std::cout << "choose a device: (which device you want to edit?)\n" << std::endl;
			std::cout << "   Name:	MAC:" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ". " << dev.name << ",	" << dev.MAC;
			}
			std::cout << "" << std::endl; //new Line

			do {
				std::cin >> device;
				std::cin.clear();
				std::cin.ignore();
			} while (device < 1 || device > device_list.size());

			std::cout << "-----> Write on " << device_list[device-1]->name << ":" << std::endl;

			unsigned char mac[6] = {device_list[device-1]->MAC[0], device_list[device-1]->MAC[1], device_list[device-1]->MAC[2], device_list[device-1]->MAC[3], device_list[device-1]->MAC[4], device_list[device-1]->MAC[5]};

			std::cout << "choose parameter: (which one you want to edit?)\n" << std::endl;
			std::cout << "1. Set IP-Adress? - Press 1:" << std::endl;
			std::cout << "2. Set Device-Name? - Press 2:\n" << std::endl;

			do{
				std::cin >> option;
				std::cin.clear();
				std::cin.ignore();
			} while (option < 1 || option > 2);	//TODO: Was wenn keine Options?

			//always equal in Set-Request
			struct dcpHeader dcpHeader_setReq(serviceID_setReq, serviceType_Req, xID, respDelayFactor_answer, dataLength_setIP);
			struct dcpDataHeader dcpDataHeader_setReq(0x00, 0x00, lengthIP, status);
			
			if(option == 1) { //edit IP-Adress
				std::cout << "-----> Edit IP-Adress:\n" << std::endl;	
				dcpDataHeader_setReq.option = 0x01;
				dcpDataHeader_setReq.subOption = 0x02;

				std::cout << "Please enter 4 numbers between 0 and 255 for new IP-Adress" << std::endl;
				std::cout << "& another 4 numbers between 0 and 255 for new subnetmasc!" << std::endl;
				std::cout << "& another 4 numbers between 0 and 255 for new standardGateway!" << std::endl;
				
				unsigned int tmp;
				unsigned char ipData[48];

				for(int i = 0; i<12; i++) {
					do {
						std::cin >> tmp;
						std::cin.clear();
						std::cin.ignore();	
					} while (tmp < 0 || tmp > 255);
					
					ipData[0+i] = tmp;
					
					if(i == 3) std::cout << "subnetMasc:" << std::endl;
					if(i == 7) std::cout << "standardGateway:" << std::endl;
				}
				std::cout << std::endl;
				//send DCP-Freme to set new IP-Param
				sendDCPFrame(mac, etherType_profiNET, frameID_setReq, dcpHeader_setReq, dcpDataHeader_setReq, ipData);

				//Change IP-Param in local device-List
				memcpy(&device_list[device-1]->ipParam, ipData, 12*sizeof(u_char));

				std::cout << "New IP-Data of device: " << device_list[device-1]->name << ":" << std::endl;
				std::cout << device_list[device-1]->ipParam << std::endl;

			} else if(option == 2) { //edit Device-Name
				std::cout << "-----> Edit Device-Name:\n" << std::endl;
				dcpDataHeader_setReq.option = 0x02;
				dcpDataHeader_setReq.subOption = 0x02;

				std::cout << "Please enter a new Name!" << std::endl;

				std::string newDeviceName;

				std::cin >> newDeviceName;

				unsigned int len = newDeviceName.size() + 2; // + length of status

				unsigned char *deviceName = new unsigned char[len+1];
				strcpy((char *)deviceName , newDeviceName.c_str());

				dcpDataHeader_setReq.length[0] = len / 256;
				dcpDataHeader_setReq.length[1] = len % 256;

				unsigned int tmpLength = 4 + len;
				if(len %2 == 1) {
					tmpLength ++; //increment
					dcpHeader_setReq.dcpDataLength[0] = tmpLength / 256;
					dcpHeader_setReq.dcpDataLength[1] = tmpLength % 256;
				} else {
					dcpHeader_setReq.dcpDataLength[0] = tmpLength / 256;
					dcpHeader_setReq.dcpDataLength[1] = tmpLength % 256;
				}
				std::cout << std::endl;

				//send DCP-Freme to set new device-Name
				sendDCPFrame(mac, etherType_profiNET, frameID_setReq, dcpHeader_setReq, dcpDataHeader_setReq, deviceName);

				//Change Device-Name in local device_list
				device_list[device-1]->name = newDeviceName;
				std::cout << "New Device_Name: " << device_list[device-1]->name << "\n" <<std::endl;

			} else {
				std::cout << "option not valid!" << std::endl;
				std::cout << "Please try again!\n" << std::endl;
			}

		} else if(decision == 5) {
			std::cout << "-----> RPC-Read:\n" << std::endl;
			
			int device;

			std::cout << "choose a device: (from which device you want to read?)\n" << std::endl;
			std::cout << "   Name:	IP-Adress:" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ". "  << dev.name << "	" << std::dec
											<< static_cast <unsigned> (dev.ipParam.ip[0]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[1]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[2]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[3]) << std::endl;
			}
			std::cout << "" << std::endl; //new Line

			do {
				std::cin >> device;
				std::cin.clear();
				std::cin.ignore();
			} while (device < 1 || device > device_list.size());

			std::cout << "-----> Read from " << device_list[device-1]->name << ":" << "\n" << std::endl;
			
			unsigned char ip[4] = {device_list[device-1]->ipParam.ip[0], device_list[device-1]->ipParam.ip[1], device_list[device-1]->ipParam.ip[2], device_list[device-1]->ipParam.ip[3]};

			//hardcodet ObjectUUID
			uu_id * oUUID= new uu_id(device_list[device-1]) ;
			uu_id * iUUID= new uu_id(&(device_list[device-1]->devRole)); //hier eigene Rolle? (Supervisor?)
			uu_id * aUUID= new uu_id();
			
			rpc_Header * testHeader=new rpc_Header(oUUID, iUUID,  aUUID);
			//testHeader->packetType=0x01; //Ping
			
			NRDData * nrdData = new NRDData;
			
			BlockHeader * blockHeader = new BlockHeader;
			IODHeader * iodHeader = new IODHeader(blockHeader);
			
			//Data to buffer:
			unsigned char* data = (unsigned char*)malloc(RPC_HEADER_LENGTH + NRD_DATA_LENGTH + IOD_HEADER_LENGTH);
			unsigned char* header = testHeader->toBuffer(); 
			unsigned char* nrd_data = nrdData->toBuffer();
			unsigned char* iod_header = iodHeader->toBuffer();
			
			memcpy(data, header, RPC_HEADER_LENGTH*sizeof(u_char));
			memcpy(&data[RPC_HEADER_LENGTH], nrd_data, NRD_DATA_LENGTH*sizeof(u_char)); 
			memcpy(&data[RPC_HEADER_LENGTH + NRD_DATA_LENGTH], iod_header, IOD_HEADER_LENGTH*sizeof(u_char));
			
			sendUDPFrame(ip, data, RPC_HEADER_LENGTH + NRD_DATA_LENGTH + IOD_HEADER_LENGTH);

			usleep(1000000);			
			
			std::cout << "\nDevice " << device_list[device-1]->name << " updated:" << std::endl;
			std::cout<<device_list[device-1]->slots<<std::endl;

		} else if(decision == 6) {
			std::cout << "-----> RPC-Write:\n" << std::endl;
			
			int device;

			std::cout << "choose a device: (from which device you want to read?)\n" << std::endl;
			std::cout << "   Name:	IP-Adress:" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ". "  << dev.name << "	" << std::dec
											<< static_cast <unsigned> (dev.ipParam.ip[0]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[1]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[2]) << ":"
											<< static_cast <unsigned> (dev.ipParam.ip[3]) << std::endl;
			}
			std::cout << "" << std::endl; //new Line

			do {
				std::cin >> device;
				std::cin.clear();
				std::cin.ignore();
			} while (device < 1 || device > device_list.size());

			std::cout << "-----> Write on " << device_list[device-1]->name << ":\n" << std::endl;

			unsigned char ip[4] = {device_list[device-1]->ipParam.ip[0], device_list[device-1]->ipParam.ip[1], device_list[device-1]->ipParam.ip[2], device_list[device-1]->ipParam.ip[3]};
			
			//*********BUILD CONNECT-FRAME*************
			//Längen:
			int afterNRDDataSize = IOD_CONNECT_REQ_LENGTH + etherNameLength;
			int afterRPCDataSize = afterNRDDataSize + NRD_DATA_LENGTH;

			//create UUID´s for rpc-Header
			uu_id * oUUID= new uu_id(device_list[device-1]) ;
			uu_id * iUUID= new uu_id(&(device_list[device-1]->devRole));
			uu_id * aUUID= new uu_id();
			//create rpc-Header
			rpc_Header * testHeader = new rpc_Header(oUUID, iUUID,  aUUID);
			//change Operation (in rpc-Header): Connect (0x0000)
			testHeader->operationNumber[0] = testHeader->operationNumber[1] = 0x00;
			//change Length in rpc-Header
			testHeader->lengthOfBody[0] = afterRPCDataSize / 256;
			testHeader->lengthOfBody[1] = afterRPCDataSize % 256;
			
			//create NRDData-Header
			NRDData * nrdData = new NRDData(maxBodyLengthRPC, afterNRDDataSize);
			

			ARBlockRequest * ConnectRequest = new ARBlockRequest();
			
			//Data to Buffer
			unsigned char* data = (unsigned char*)malloc(RPC_HEADER_LENGTH + NRD_DATA_LENGTH + afterNRDDataSize);
			unsigned char* header = testHeader->toBuffer(); 
			unsigned char* nrd_data = nrdData->toBuffer();
			unsigned char* connectRequest = ConnectRequest->toBuffer();
			
			memcpy(data, header, RPC_HEADER_LENGTH*sizeof(u_char));
			memcpy(&data[RPC_HEADER_LENGTH], nrd_data, NRD_DATA_LENGTH*sizeof(u_char)); 
			memcpy(&data[RPC_HEADER_LENGTH + NRD_DATA_LENGTH], connectRequest, (afterNRDDataSize) * sizeof(u_char));
			
			sendUDPFrame(ip, data, RPC_HEADER_LENGTH + NRD_DATA_LENGTH + afterNRDDataSize);

		} else {
			std::cout << "decision not valid!" << std::endl;
			std::cout << "Please try again!" << std::endl;
		}
	}
	return 0;
}
