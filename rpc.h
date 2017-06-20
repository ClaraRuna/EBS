//include-guard 
#pragma once
#ifndef RPC_H
#define RPC_H

#include <iostream>
#include <iomanip>
#include "dcp.h"

#define MAX_BUFFER 1024

typedef struct uu_id{ //16 Byte	
	
	u_char field1[4];      //32 Bit
	u_char field2[2];     //16 Bit
	u_char field3[2];     //16 Bit 
	u_char field4[8]; //64 Bit
	
	//konstruktoren
	//default: ActivityUUID, weil frei bestimmbar, hat einfach festen Wert in Feldern
	uu_id();
	//ObjectUUID
	uu_id(device * object);
	//InterfaceUUID
	uu_id(device_role * d);
	
	unsigned char * toBuffer();
}UUID;

typedef struct rpc_Header{
	u_char version;
	u_char packetType; //Request, Response, Ping, etc
	u_char flags1;
	u_char flags2;
	u_char dRep[3]; //Representation of datatypes 
	u_char serialHigh; //Das höherwertige Byte der Fragmentnr des Aufrufs? (=fragmentNr[1]?)
	//UUIDs je 16 Byte
	struct uu_id * objectUUID; 		
	struct uu_id * interfaceUUID;		
	struct uu_id * activityUUID; 		
	u_char serverBootTime[4]; 	//req -> 0 ; resp -> zeit
	u_char interfaceVersion[4];	//=1
	static long headerCount;	//total nr of headers created, needed for counting the seq Nr
	u_char sequenceNumber[4];		//bei mehr als 255 anfragen muss wahrsch m memcopy und manueller pointerarithmetik gearbeitet werden
	u_char operationNumber[2];
	u_char interfaceHint[2]; 	//fest:0xFFFF
	u_char activityHint[2]; 	//fest:0xFFFF
	u_char lengthOfBody[2];		//Länge der dem Header folgenden Daten
	u_char fragmentNumber[2];	//bei keiner Fragmentierung 0
	u_char authentificationProtocoll;
	u_char serialLow;		//niederwertiges Byte der Fragmentnr des Aufrufs (=fragmentNr[0]?)
	

	//konstruktoren
	rpc_Header(uu_id * oUUID, uu_id * iUUID, uu_id * aUUID);
	rpc_Header();	
	void construct();	//setzt-standard werte f variablen
		
	unsigned char *  toBuffer();		//gibt rpc_Header in Buffer
	
}RPCHEADER;

#endif