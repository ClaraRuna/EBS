#include "rpc.h"

unsigned char *  rpc_Header::toBuffer(){
	unsigned char* buffer = (unsigned char*)malloc(MAX_BUFFER);
	
	buffer[0]=version;
	buffer[1]=packetType;
	buffer[2]=flags1;
	buffer[3]=flags2;
	buffer[4]=dRep[0];
	buffer[5]=dRep[1];
	buffer[6]=dRep[2];

	
	return buffer;
}