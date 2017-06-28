#include "header.h"

#include <netinet/in.h>
#include <iostream>
#include <string.h>

#define BUF_SIZ		1024
#define LOCAL_PORT	0x8894
#define REMOTE_PORT 0x8894

int sockUDP;

void createUDPsocket() {
	/*struct used for binding the socket to a local address*/
	struct sockaddr_in host_address;	
	
	if((sockUDP = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("cannot create socket for sending UDP-Frames\n");
	}
	/*init the host_address the socket is beeing bound to*/
	memset((void*)&host_address, 0, sizeof(host_address));
	/*set address family*/
	host_address.sin_family=PF_INET;
	/*accept any incoming messages:*/
	host_address.sin_addr.s_addr=INADDR_ANY;
	/*the port the socket i to be bound to:*/
	host_address.sin_port=htons(LOCAL_PORT);
	
	/*bind it...*/
	if (bind(sockUDP, (struct sockaddr*)&host_address, sizeof(host_address)) < 0) {
		perror("cannot bind socket for sending UDP-Frames\n");
	}
}

void sendUDPFrame(unsigned char destIP[], unsigned char data[], int dataSize) {
	char buffer[BUF_SIZ]; 					/*the message to send*/
	struct sockaddr_in target_host_address;	/*the receiver's address*/
	unsigned char* target_address_holder;	/*a pointer to the ip address*/

	/*init target address structure*/
	target_host_address.sin_family=PF_INET;
	target_host_address.sin_port=htons(REMOTE_PORT);
	target_address_holder=(unsigned char*)&target_host_address.sin_addr.s_addr;
	target_address_holder[0] = destIP[0];
	target_address_holder[1] = destIP[1];
	target_address_holder[2] = destIP[2];
	target_address_holder[3] = destIP[3];

	/*fill message with data....*/
	for (int j = 0; j < dataSize; j++) {
		buffer[j] = data[j];
	}
	
	std::cout<<"packet data"<< std::endl;
	for (int j = 0; j < dataSize; j++) {
		std::cout << std::hex << buffer[j]<<" ";
	}
	std::cout << std::endl; //new Line

	/*send it*/
	std::cout<< "sendto returns " << sendto(sockUDP, buffer, dataSize, 0, (struct sockaddr*)&target_host_address, sizeof(struct sockaddr)) <<std::endl;
}