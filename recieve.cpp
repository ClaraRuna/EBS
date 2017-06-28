#include <string.h>
#include <netinet/ether.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>

#include "header.h"

int sockrec;//socket for recieving

void createRECSocket() {
	//for recieving DCP-Frames
	if ((sockrec = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		perror("cannot create socket for recieveing Frames\n");
	}
}

struct device* parseResp(unsigned char* buffer, device*d){
	//temp is at current reading position
	int temp=0;
	unsigned short data_length =0;
	
	//auf korrekten DCP-Header prüfen:
	//16:service-ID = 0x05, 17:service-Type =0x01 
	//little/big endian? evtl buffer [17]=0x80
	if (buffer[16]==0x03 && buffer [17]==0x01 ){
		std::cout<<"Get Request"<<std::endl;
	}
	else if ((buffer[16] == 0x05 && buffer [17] == 0x01)){
		d->MAC[0]=buffer[6];
		d->MAC[1]=buffer[7];
		d->MAC[2]=buffer[8];
		d->MAC[3]=buffer[9];
		d->MAC[4]=buffer[10];
		d->MAC[5]=buffer[11];
	}

	//byte 24+25: data length, this is ugly
	data_length=buffer[25]+buffer[24]*256;
	
	//temp auf erstes dcp-data-header feld setzen
	temp=26;
	while (temp<data_length+25){
		struct dcp_data_header dcp_dh;
		dcp_dh.opt=buffer[temp];
		dcp_dh.subopt=buffer[temp+1];
		dcp_dh.length=buffer[(temp+2)]*256+buffer[temp+3];
		//so something usefull
		//das klappt hier eh noch nicht so...
		
		//std::cout<<dcp_dh;
			//switch-case anweisung, die nach opt und subopt filtert
		//entsprechende info in das device schreibt, dessen pointer die fkt am ende zurückgibt
		
		//je temp + 4 (header) + 2 (blockinfo) = beginn der info
		switch (dcp_dh.opt){
			//IP
			case 1: switch(dcp_dh.subopt) {
				case 1: //Mac-Adresse -> TODO mac adresse aus header ggf überschreiben oder wat?? 
					break;
				case 2: //IP-Parameter
					for (int i =0; i<4; i++){
						d->ipParam.ip[i]=static_cast<int>(buffer[temp+ i +6]);
						d->ipParam.subnet[i]=static_cast<int>(buffer[temp + i + 10]);
						d->ipParam.gateway[i]=static_cast<int>(buffer[temp + i +14]);
					}
				default: break;
			} break;
			//DeviceProp
			case 2: switch (dcp_dh.subopt){
				case 1: //Hersteller
					d->vendor="";
					for (int i = temp+6; i<temp+4+dcp_dh.length; i++){
						d->vendor+= static_cast<char> (buffer [i]);
					}
					break;
				case 2: //Stationsname
						//temp+4header+2blockinfo
					d->name="";
					for (int i = temp+6; i<temp+4+dcp_dh.length; i++){
						d->name+= static_cast<char> (buffer [i]);
					}
					break;
				case 3: //DeviceID: 2 byte blockinfo, 2 byte hersteller id, 2 byte stationsid
					d->vendor_id[0]=buffer[temp+6]; //4 header, 2 info
					d->vendor_id[1]=buffer[temp+7];
					d->device_id[0]=buffer[temp+8];
					d->device_id[1]=buffer[temp+9];
					break;
				case 4: //Geräterolle
					d->devRole=static_cast <device_role> (buffer[temp+6]);
					break;
				case 5: //Optionen
					//std::cout << "temp= " <<temp << std::endl;
					//std::cout << "dev " << d->name <<" bietet folgende optionen:" << std::endl;
					d->options.clear();
					for (int i= temp+6; i< temp+4+dcp_dh.length-1; i++){
						struct option * o;
						o=new option;
						o->opt=buffer[i];
						o-> subopt= buffer[i+1];
						i++;
						d->options.push_back(o);							//std::cout << "opt" << static_cast <unsigned> (buffer[i]) << " subopt" << static_cast <unsigned> (buffer[i+1]) << std::endl;
						//i++;
						//std::cout << "i" << i << std::endl;
					}
					break;
				case 6: //Aliasname d Device
				default:break;
			}
			//DHCP
			case 3: switch (dcp_dh.subopt){
				//siehe IEC 61158-6-10-4.3.1.4.4
			}
			//Control
			case 5: switch (dcp_dh.subopt){
				//Start
				case 1:
				//Stop 
				case 2:
				//Signal
				case 3:
				//Response
				case 4:
				//Factory Reset
				case 5:
				default: break;
			}
			// DevInitiative
			case 6: break;
			//All Selector
			case 0xFF: break;
		}
		temp=temp+4+dcp_dh.length;
		//fill padding
		if (dcp_dh.length % 2 ==1) temp+=1;
	}
	//erhaltene deviceinfos ausgeben
	//std::cout<<d<<std::endl;
	return d;
}

void recieveResponse(std::vector <struct device*> *device_list) {
	std::cout<<"start recieving ProfiNET-Frames....\n"<<std::endl;
	while (1) {
		unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_LEN); //Buffer for ethernet frame
		int length = 0; //length of the received frame
		
		length = recvfrom(sockrec, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
		if (length == -1) {
			printf("errorhandling ....\n");
		}	
		if (buffer[12]==0x88 && buffer [13]==0x92 //dcp-paket
			&& buffer[18]==0x42 && buffer[19]==0x42 && buffer[20]==0x42 && buffer[21]==0x42) { //mit korrekter xid

			struct device * d = NULL;
			//check if device already exists
			if (device_list->size() > 0){
				for (int i=0; i<device_list->size(); i++){
					device * dev = device_list->at(i);
					bool equalMAC = true;
					for (int j=0; j<6; j++){
						if (dev->MAC[j] != buffer[6+j]) equalMAC=false;
					}
				if (equalMAC==true) d=dev;
				}
			}

			if (d==NULL && buffer[16]==0x05 && buffer [17]==0x01) { //IdentResp
				d=new device;
				device_list->push_back(d);
			}
			if (d != NULL) {
				parseResp(buffer, d);				
			}
		}
	}
}