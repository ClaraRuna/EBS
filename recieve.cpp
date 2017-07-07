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

void parseRPCBlock(unsigned char* buffer, device*d){
	//std::cout<<"ab hier könnten fehler mit der übergabe des buffers von parseRPCResponse zu parseRPCBlock zusammenhängen" <<std::endl;
	switch (buffer[0]){
		case 0x80: std::cout<<"IODReadResponseHeader (wird nicht ausgewertet)"<<std::endl; break;
		case 0x00:
			switch(buffer[1]){
				case 0x09: std::cout<<"IODReadRequestHeader"<<std::endl; break;
					//korrekter check wäre nach table 502
				case 0x20: std::cout<<"I&M0"<<std::endl; break;
				case 0x30: {std::cout<<"I&M0 FilterDataSubModul (wird ausgewertet)"<<std::endl; 
					int bufferPointer =0;
					unsigned short nrOfModules;
					unsigned short stemp;
					unsigned long ltemp;
					
					memcpy (&stemp, &buffer [12], sizeof(unsigned short));
					nrOfModules = ntohs (stemp);
					//iterate over nr of modules, add offset each time
					std::cout<<"nrOfModules: " <<nrOfModules<<std::endl;
					for (int i=0; i<nrOfModules; i++){
						unsigned short slotNr;					
						unsigned long moduleIdentNr;
						unsigned short nrOfSubmodules;
						
						
						memcpy (&stemp, &buffer[bufferPointer], sizeof (unsigned short));
						slotNr=ntohs (stemp);
						
						memcpy (&ltemp, &buffer[bufferPointer+2], sizeof (unsigned long));
						moduleIdentNr=ntohs (ltemp);
						
						memcpy (&stemp, &buffer[bufferPointer+6], sizeof (unsigned short));
						nrOfSubmodules=ntohs (stemp);
						
						bufferPointer=bufferPointer+8 ; 	//bufferPointer um Länge des soeben ausgelesenen parts erhöhen
						
						std::cout<<"slotNr: " <<slotNr << "moduleIdentNr: " << moduleIdentNr << "nrOfSubmodules: " << nrOfSubmodules <<std::endl;
						
							for (int i=0; i<nrOfSubmodules; i++){
								unsigned short subslotNr;
								unsigned long subModuleIdentNr;
								
								memcpy (&stemp, &buffer[bufferPointer], sizeof (unsigned short));
								subslotNr = ntohs (stemp);
								
								memcpy (&ltemp, &buffer[bufferPointer + 2], sizeof (unsigned long));
								subModuleIdentNr = ntohs (ltemp);
								
								bufferPointer=bufferPointer+6;		//bufferPointer um Länge des soeben ausgelesenen parts erhöhen
								
								std::cout<< "subslotNr: "  << subslotNr << "subModuleIdentNr: " << subModuleIdentNr << std::endl;
							}
						}
					break;
					}
				case 0x31: std::cout<<"I&M0 FilterDataModul"<<std::endl; break;
				case 0x32: std::cout<<"I&M0 FilterDataDevice"<<std::endl; break;
			}	
	}
}
//statt void muss hier dann noch der i&m0 datentyp hin, der noch zu implementieren ist
void parseRPCResponse (unsigned char* buffer, device*d){
	//NRDDataRequest beginnt bei 122
	int totalLength = 122 + static_cast <unsigned> (buffer[116]) +static_cast <unsigned> ( buffer[117]*256);
	std::cout << "total length is " << std::dec<< totalLength << std::endl;
	int temp = 142; 	//bei 142 beginnt der erste Block
	//int blockLength = buffer[temp +2]*256 + buffer[temp+3];
	while (temp<=totalLength){
		std::cout<<"blocktype: " << static_cast <unsigned> (buffer[temp])  << ":" << static_cast <unsigned> (buffer[temp+1]) <<std::endl; 
		int blockLength = buffer[temp +2]*256 + buffer[temp+3];
		if (blockLength ==0) break;
		std::cout<< "block gefunden bei " << temp << ": Länge: " << blockLength <<std::endl;
		parseRPCBlock(&buffer[temp], d);
		temp=temp+blockLength+4;  //+4 weil blocktype [2] und blocklength[2] nicht in die länge zählen??
	}	
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
		//pnio-paket
		//port checken: udp-paket mit in-port 0x8894
		//hier noch mehr checken, falls andere pakete mit den zahlen im buffer rein kommen (unwahrscheinlich)
		else if (buffer[36]==0x88 && buffer[37]==0x94 && buffer[43]==0x02){
			std::cout << "response on port 0x8894 from ";
			for (int i=26; i<30; i++) {
				std::cout << static_cast <int> (buffer[i]) <<"." ;
			}
			std::cout<<"(ip)"<<std::endl;
			//pakettyp und error codes checken (122-125)
			if ((buffer[122]!=0x00 || buffer[123]!=0x00 || buffer[124]!=0x00 || buffer[125]!=0x00) ){
				std::cout<<"rpc-response signals error: \n errorcode 2: " << static_cast <unsigned> (buffer[122]) <<
										"\n errorcode 1: " << static_cast <unsigned> (buffer [123]) <<
										"\n errordecode: " <<  static_cast <unsigned> (buffer[124]) <<
										"\n errorcode: " <<   static_cast <unsigned> (buffer[125]) << std::endl;
			}
			//object id checken, um richtiges dev aus liste zu suchen (50-65)
			else {	
				//pointer to current device
				device * d = NULL;		
				//buffer 62-63=dev id, buffer 64-65= vend_id -> checken
				for (int i=0; i<device_list->size(); i++){
					device * dev = device_list->at(i);
					if (dev->MAC[0]==buffer[6] &&
							dev->MAC[1]==buffer[7] &&
							dev->MAC[2]==buffer[8] &&
							dev->MAC[3]==buffer[9] &&
							dev->MAC[4]==buffer[10] &&
							dev->MAC[5]==buffer[11] ) {
						d = dev;
					}
				}
				if (d == NULL){
					std::cout << "rpc-response konnte keinem device zugeordnet werden und wird ignoriert" <<std::endl;
				}
				else{
					std::cout << "rpc-response wurde " <<d->name << " zugeordnet" <<std::endl;
					parseRPCResponse(buffer, d);
				}
			}
			
		}
	}
}