#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <iostream>
#define BUF_SIZE 1000
#define LOCAL_PORT 8894
#define REMOTE_PORT 8894

/*socketdescriptor*/
int s;
						
int create_udp_socket(int port) {				
	/*struct used for binding the socket to a local address*/
	struct sockaddr_in host_address;	
	
	/*create the socket*/
	s=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0) { /*errorhandling ....*/}
	
	/*init the host_address the socket is beeing bound to*/
	memset((void*)&host_address, 0, sizeof(host_address));
	/*set address family*/
	host_address.sin_family=PF_INET;
	/*accept any incoming messages:*/
	host_address.sin_addr.s_addr=INADDR_ANY;
	/*the port the socket i to be bound to:*/
	host_address.sin_port=htons(port);
	
	/*bind it...*/
	if (
	   bind(s, (struct sockaddr*)&host_address, sizeof(host_address)) < 0
	   ) {
		std::cout<<"errorhandling..."<<std::endl;
	}
	return s;
}

int main(){
	char buffer[BUF_SIZE]; 			/*the message to send*/
	struct sockaddr_in target_host_address;	/*the receiver's address*/
	unsigned char* target_address_holder;	/*a pointer to the ip address*/

	s = create_udp_socket(LOCAL_PORT);
	if (s == -1) {std::cout << "errorhandling....." <<std::endl;}

	/*init target address structure*/
	target_host_address.sin_family=PF_INET;
	target_host_address.sin_port=htons(REMOTE_PORT);
	target_address_holder=(unsigned char*)&target_host_address.sin_addr.s_addr;
	target_address_holder[0]=0xAC;
	target_address_holder[1]=0x10;
	target_address_holder[2]=0x01;
	target_address_holder[3]=0xD5;

	/*fill message with random data....*/
	for (int j = 0; j < BUF_SIZE; j++) {
		buffer[j] = 0x42;
	}
	
	std::cout<<"packet data"<< std::endl;
	for (int j = 0; j < BUF_SIZE; j++) {
		std::cout<<buffer[j]<<" ";
	}
	std::cout << std::endl;

	/*send it*/
	std::cout<< "sendto returns " << sendto(s, buffer, BUF_SIZE, 0, (struct sockaddr*)&target_host_address, sizeof(struct sockaddr)) <<std::endl;


}