#include "header.h"

#include <linux/if_packet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <iostream>
#include <netinet/in.h>


#define BUF_SIZ		1024
#define DEFAULT_IF	"enx9cebe808bd01"

int sockfd; //socket for sending DCP-Frames

char ifName[IFNAMSIZ]; //Name of Ethernetport

void createRAWSocket() {
	//for sending DCP-Frames
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("cannot create socket for sending DCP-Frames\n");
	}
}

void setIfName(char *argv[]) {
	if(argv != NULL)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);
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
	if(dcph.serviceID != 0x03) { //unsigned char serviceID_getReq = 0x03;
		sendbuf[tx_len++] = dcpdh.length[0];
		sendbuf[tx_len++] = dcpdh.length[1];
	}

	//data
	if(dcph.serviceID == 0x04) { //unsigned char serviceID_setReq = 0x04;
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