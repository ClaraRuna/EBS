/* g++ -std=c++11 -pthread -o test sendIdentifyReq.c
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <vector>
#include <thread>
#include <unistd.h>
#include "dcp.h"

#define DEFAULT_IF	"enx9cebe808bd01"
#define BUF_SIZ		1024

//socket
int sockfd; //socket for sending
int sockrec;//socket for recieving

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

unsigned char xID[4] = {0x42, 0x42, 0x42, 0x42}; //zur identifikation - dynamisch?

unsigned char respDelayFactor_answer[2] = {0x00, 0x00};
unsigned char respDelayFactor_withoutImpact[2] = {0x00, 0x01};
//0x0002 - 0x1900 → mit Aufschlag

unsigned char dataLength[2] = {0x00, 0x04};
unsigned char dataLength_getReq[2] = {0x00, 0x02};
unsigned char dataLength_setIP[2] = {0x00, 0x12};

unsigned char status[2] = {0x00, 0x00};
unsigned char length[2] = {0x00, 0x00};
unsigned char lengthIP[2] = {0x00, 0x0E};

//Declaration of Functions
void createRAWEthernetSocket();
void sendDCPFrame(unsigned char destMAC[], unsigned char etherType[], unsigned char frameID[], struct dcpHeader, struct dcpDataHeader, unsigned char data[]);
void recieveResponse();

int main(int argc, char *argv[]) {

	std::cout << "----------ProfiNET-Tool----------\n" << std::endl;

	createRAWEthernetSocket();

	std::thread first (recieveResponse);

	usleep(1000000);

	std::cout << "-----> send Identification-Request:" << std::endl;
	struct dcpHeader dcpHeader_identReq(serviceID_identReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength);
	struct dcpDataHeader dcpDataHeader_identReq(0xFF, 0xFF, length, status);
	sendDCPFrame(multicast_mac, etherType_profiNET, frameID_identRequest, dcpHeader_identReq, dcpDataHeader_identReq, 0x00);

	usleep(1000000);  //sleep 1s (in us) --> wait for answers

	std::cout << "" << std::endl; //new line
	std::cout << "Identification of " << device_list.size() << " Devices:" << std::endl;
	std::cout << "" << std::endl; //new line
	for(int i = 0; i < device_list.size(); i++) {
		std::cout << *device_list[i] <<std::endl;
	}

	int decision;

	while(1) {
		
		usleep(1000000);

		std::cout << "What you want to do next?" << std::endl;
		std::cout << "1. Hello-Request? - Press 1" << std::endl;
		std::cout << "2. Ident-Request? - Press 2" << std::endl;
		std::cout << "3. Get-Request?   - Press 3" << std::endl;
		std::cout << "4. Set-Request?   - Press 4\n" << std::endl;

		scanf("%d", &decision);

		if(decision == 1) {
			std::cout << "-----> Hello-Request:\n" << std::endl;
			struct dcpHeader dcpHeader_helloReq(serviceID_helloReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength);
			struct dcpDataHeader dcpDataHeader_helloReq(0xFF, 0xFF, length, status);
			sendDCPFrame(multicast_mac, etherType_profiNET, frameID_helloRequest, dcpHeader_helloReq, dcpDataHeader_helloReq, 0x00);	
		} else if(decision == 2) {
			std::cout << "-----> Identification-Request:\n" << std::endl;
			sendDCPFrame(multicast_mac, etherType_profiNET, frameID_identRequest, dcpHeader_identReq, dcpDataHeader_identReq, 0x00);
			usleep(1000000);
			for(int i = 0; i < device_list.size(); i++) {
				std::cout << *device_list[i] <<std::endl;
			}
		} else if(decision == 3) {
			std::cout << "-----> Get-Request:\n" << std::endl;
			int device, param;
			std::cout << "choose a device: (from which device you want to read?)\n" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ". " << dev.name << ":		" << std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[0])<<":"
												<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[1])<<":"
												<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[2])<<":"
												<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[3])<<":"
												<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[4])<<":"
												<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[5]) <<std::endl;
			}

			std::cout << "" << std::endl;
			scanf("%d", &device);
			unsigned char mac[6] = {device_list[device-1]->MAC[0], device_list[device-1]->MAC[1], device_list[device-1]->MAC[2], device_list[device-1]->MAC[3], device_list[device-1]->MAC[4], device_list[device-1]->MAC[5]};

			std::cout << "\nchoose option and subotion: (which parameter you want to read?)\n" << std::endl;
			
			std::cout<<device_list[device-1]->options<<std::endl;
			
			scanf("%i", &param);
			
			unsigned char option = device_list[device - 1]->options[param - 1]->opt;
			unsigned char subOption = device_list[device - 1]->options[param -1]->subopt;

			//always equal in Get-Request
			struct dcpHeader dcpHeader_getReq(serviceID_getReq, serviceType_Req, xID, respDelayFactor_withoutImpact, dataLength_getReq);
			struct dcpDataHeader dcpDataHeader_getReq(option, subOption, length, status);
			sendDCPFrame(mac, etherType_profiNET, frameID_getReq, dcpHeader_getReq, dcpDataHeader_getReq, 0x00);		
		
		} else if(decision == 4) {
			std::cout << "-----> Set-Request:\n" << std::endl;
			int device, option;
			std::cout << "choose a device: (which device you want to edit?)\n" << std::endl;
			for(int tmp = 0; tmp < device_list.size(); tmp++) {
				struct device dev = *device_list[tmp];
				std::cout << tmp+1 << ": " << dev.name << ":	" <<static_cast <unsigned> (dev.MAC[0])<<":"
												<<static_cast <unsigned> (dev.MAC[1])<<":"
												<<static_cast <unsigned> (dev.MAC[2])<<":"
												<<static_cast <unsigned> (dev.MAC[3])<<":"
												<<static_cast <unsigned> (dev.MAC[4])<<":"
												<<static_cast <unsigned> (dev.MAC[5]) <<std::endl;
			}
			std::cout << "" << std::endl;
			scanf("%d", &device);
			unsigned char mac[6] = {device_list[device-1]->MAC[0], device_list[device-1]->MAC[1], device_list[device-1]->MAC[2], device_list[device-1]->MAC[3], device_list[device-1]->MAC[4], device_list[device-1]->MAC[5]};

			std::cout << "choose parameter: (which one you want to edit?)\n" << std::endl;
			std::cout << "1. Set IP-Adress? - Press 1:" << std::endl;
			std::cout << "2. Set Device-Name? - Press 2:\n" << std::endl;

			scanf("%i", &option);
			
			//always equal in Set-Request
			struct dcpHeader dcpHeader_setReq(serviceID_setReq, serviceType_Req, xID, respDelayFactor_answer, dataLength_setIP);
			struct dcpDataHeader dcpDataHeader_setReq(0x00, 0x00, lengthIP, status);
			
			if(option == 1) { //edit IP-Adress
				std::cout << "-----> Edit IP-Adress:" << std::endl;	
				dcpDataHeader_setReq.option = 0x01;
				dcpDataHeader_setReq.subOption = 0x02;

				std::cout << "Please enter 4 numbers between 0 and 255 for new IP-Adress" << std::endl;
				std::cout << "& another 4 numbers between 0 and 255 for new subnetmasc!" << std::endl;
				std::cout << "& another 4 numbers between 0 and 255 for new standardGateway!" << std::endl;
				
				unsigned int tmp;
				unsigned char data[48];

				for(int i = 0; i<12; i++) {
					scanf("%u", &tmp);
					if(tmp >= 0 && tmp <= 255) {
						data[0+i] = tmp;
					} else {
						printf("invalid! again!\n");
						i--;
					}
					if(i == 3) printf("subnetmasc:\n");
					if(i == 7) printf("standardGateway:\n");
				}		
				sendDCPFrame(mac, etherType_profiNET, frameID_setReq, dcpHeader_setReq, dcpDataHeader_setReq, data);


			} else if(option == 2) { //edit Device-Name
				std::cout << "-----> Edit DeviceName:" << std::endl;
				dcpDataHeader_setReq.option = 0x02;
				dcpDataHeader_setReq.subOption = 0x02;

				std::cout << "Please enter a new Name!" << std::endl;

				std::string name;

				std::cin >> name;

				unsigned int len = name.size() + 2;

				unsigned char *deviceName = new unsigned char[len+1];
				strcpy((char *)deviceName , name.c_str());

				dcpDataHeader_setReq.length[0] = len / 256;
				dcpDataHeader_setReq.length[1] = len % 256;

				unsigned int tmpLength = 4 + len;
				if(len %2 == 1) {
					tmpLength ++;
					dcpHeader_setReq.dcpDataLength[0] = tmpLength / 256;
					dcpHeader_setReq.dcpDataLength[1] = tmpLength % 256;
				} else {
					dcpHeader_setReq.dcpDataLength[0] = tmpLength / 256;
					dcpHeader_setReq.dcpDataLength[1] = tmpLength % 256;
				}
				
				sendDCPFrame(mac, etherType_profiNET, frameID_setReq, dcpHeader_setReq, dcpDataHeader_setReq, deviceName);

			} else {
				std::cout << "option not valid!" << std::endl;
			}


		} else {
			std::cout << "decition not valid!" << std::endl;
		}
	}
	return 0;
}

void createRAWEthernetSocket() {
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		std::cout << "CreateSocketError" << std::endl;
	}
	if ((sockrec = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
	    perror("socket\n");
	}
}

void sendDCPFrame(unsigned char mac[], unsigned char etherType[], unsigned char frameID[], struct dcpHeader dcph, struct dcpDataHeader dcpdh, unsigned char data[]) {

	struct ifreq if_idx;
	struct ifreq if_mac;
	//counter for sendbuf
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	
	struct ether_header *eh = (struct ether_header *) sendbuf;
	//struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ];
	
	strcpy(ifName, DEFAULT_IF);

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");


	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = mac[0];
	eh->ether_dhost[1] = mac[1];
	eh->ether_dhost[2] = mac[2];
	eh->ether_dhost[3] = mac[3];
	eh->ether_dhost[4] = mac[4];
	eh->ether_dhost[5] = mac[5];
	/* Ethertype field */	//--> 0x8892 verdreht
	eh->ether_type = etherType[1]*256 + etherType[0];
	
	//counter
	tx_len += sizeof(struct ether_header);

	/* Packet data */
	//frame id
	sendbuf[tx_len++] = frameID[0];
	sendbuf[tx_len++] = frameID[1];
	
	//dcp-header
	//serviceID
	sendbuf[tx_len++] = dcph.serviceID;
	//service type 
	sendbuf[tx_len++] = dcph.serviceType;
	//xid/transaction id -Y beliebig
	sendbuf[tx_len++] = dcph.xID[0];
	sendbuf[tx_len++] = dcph.xID[1];
	sendbuf[tx_len++] = dcph.xID[2];
	sendbuf[tx_len++] = dcph.xID[3];
	//response delay factor
	sendbuf[tx_len++] = dcph.respDelayFactor[0];
	sendbuf[tx_len++] = dcph.respDelayFactor[1];
	//length : 4
	sendbuf[tx_len++] = dcph.dcpDataLength[0];
	sendbuf[tx_len++] = dcph.dcpDataLength[1];
	
	//dcp-data-header
	//opt: 0xFF -> all selectors
	sendbuf[tx_len++] = dcpdh.option;
	//subopt
	sendbuf[tx_len++] = dcpdh.subOption;
	
	//length
	if(dcph.serviceID != serviceID_getReq) {
		sendbuf[tx_len++] = dcpdh.length[0];
		sendbuf[tx_len++] = dcpdh.length[1];
	}

	//data
	if(dcph.serviceID == serviceID_setReq) {
		sendbuf[tx_len++] = dcpdh.status[0];
		sendbuf[tx_len++] = dcpdh.status[1];
		for(int i = 0; i<(dcpdh.length[0]*256 + dcpdh.length[1])-2; i++) {
			sendbuf[tx_len++] = data[i];
		}
	}

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	/* Destination MAC */
	socket_address.sll_addr[0] = mac[0];
	socket_address.sll_addr[1] = mac[1];
	socket_address.sll_addr[2] = mac[2];
	socket_address.sll_addr[3] = mac[3];
	socket_address.sll_addr[4] = mac[4];
	socket_address.sll_addr[5] = mac[5];

	/* Send packet */
	if (sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
		std::cout << "send failed!" << std::endl;
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
						d->ipParam.IP[i]=buffer[temp +6];
						d->ipParam.Subnet[i]=buffer[temp + 10];
						d->ipParam.Gateway[i]=buffer[temp +14];
					}
				default: break;
			} break;
			//DeviceProp
			case 2: switch (dcp_dh.subopt){
				case 1: //Hersteller
					for (int i = temp+6; i<temp+4+dcp_dh.length; i++){
						d->vendor+= static_cast<char> (buffer [i]);
					}
					break;
				case 2: //Stationsname
						//temp+4header+2blockinfo
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

void recieveResponse() {
	std::cout<<"start recieving ProfiNET-Frames....\n"<<std::endl;
	while (1) {
		unsigned char* buffer = (unsigned char*)malloc(ETH_FRAME_LEN); //Buffer for ethernet frame
		int length = 0; //length of the received frame
		
		length = recvfrom(sockrec, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
		if (length == -1) {
			printf("errorhandling ....\n");
		}	
		if (buffer[12]==0x88 && buffer [13]==0x92) { //dcp-paket

			struct device * d = NULL;
			//check if device already exists
			if (device_list.size() > 0){
				for (int i=0; i<device_list.size(); i++){
					device * dev=device_list[i];
					bool equalMAC = true;
					for (int j=0; j<6; j++){
						if (dev->MAC[j] != buffer[6+j]) equalMAC=false;
					}
				if (equalMAC==true) d=dev;
				}
			}

			if (d==NULL && buffer[16]==0x05 && buffer [17]==0x01) { //IdentResp
				d=new device;
				device_list.push_back(d);
			}
			if (d != NULL) {
				parseResp(buffer, d);				
			}
		}
	}
}