#include <iostream>
#include <iomanip>

#define MAX_BUFFER 1024

typedef struct UUID{ //16 Byte
	//TODO
	
	unsigned char * toBuffer();
}

typedef struct rpc_Header{
	u_char version;
	u_char packetType; //Request, Response, Ping, etc
	u_char flags1;
	u_char flags2;
	u_char dRep[3]; //Representation of datatypes 
	u_char serialHigh; //Das höherwertige Byte der Fragmentnr des Aufrufs?
	//UUIDs je 16 Byte
	struct UUID objectUUID; 		
	struct UUID interfaceUUID;		
	struct UUID activityUUID; 		
	u_char serverBootTime[4]; 	//req -> 0 ; resp -> zeit
	u_char interfaceVersion[4];	//=1
	u_char sequenceNumber[4];	//
	
	

	
	//default-konstruktor
	rpc_Header(){
		version=0x04;
		flags1 = 0b00000100; //little&big endian?
		flags2 = 0b01000000; //bit 1 -> "abbruch lag am aufrufende vor" ???? was? 0 oder 1 jetzt??
		dRep[0]=0b11111111; //Encoding ENBCD & Little Endian
		dRep[1]=0x00; //IEEE Floating Point
		dRep[2]=0x00; //keine Bedeutung, sollen 0 sein
		//TODO: andere Felder setzen
	}
	
	unsigned char *  toBuffer();
}RPCHEADER;