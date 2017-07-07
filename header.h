#include "struct.h"

#ifndef HEADER_H 
#define HEADER_H
	//for recieving Frames
	void createRECSocket();
	void recieveResponse(std::vector <struct device*> *device_list);

	//for sending DCP-Frames
	void createRAWSocket();
	void setIfName(char *argv[]);
	void sendDCPFrame(unsigned char destMAC[], unsigned char etherType[], unsigned char frameID[], struct dcpHeader, struct dcpDataHeader, unsigned char data[]);

	//for sending UDP-Frames (RPC)
	void createUDPsocket();
	void sendUDPFrame(unsigned char destIP[], unsigned char data[], int dataSize);

	//vor buliding rpc-Frames
	void setEtherName(char *argv[]);
#endif