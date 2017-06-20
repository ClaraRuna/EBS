#include <iostream>
#include <string.h> 
#include "rpc.h"

#define RPC_HEADER_LENGTH 80

//initialize static variable with 0;
long rpc_Header::headerCount=0; 

//default: activity, beliebig, dient nur identifikation
uu_id::uu_id():
		field1{0x42, 0x42,0x42,0x42},
		field2{0x42, 0x42}     ,     
		field3{0x42, 0x42},
		field4{0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42}
{
}

uu_id::uu_id(device*  object):
		field1{0xDE, 0xA0,0x00,0x00},
		field2{0x6C, 0x97}     ,     
		field3{0x11, 0xD1},
		field4{0x82, 0x71, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
{
	field4[2]=0; 	//höherwertiges byte der Instanz -> was da rein muss is mir noch unklar
	field4[3]=0;	//niederwertiges byte der Instanz 
	
	//reihenfolge tauschen?
	field4[4]=object->device_id[0];
	field4[5]=object->device_id[1];	

	
	field4[6]=object->vendor_id[0];
	field4[7]=object->vendor_id[1];
}

//konstruktor für interfaceUUID is noch n dummy, da interface noch nicht implementiert ist
//alles, wo 0x00 steht muss noch ausgefüllt werden -> allgemein unsicher, wie interface sinnvoll umzusetzen wäre

/*
feld1
IO-Device → DEA00001
IO-Controller → DEA00002
IO-Supervisor → DEA00003
IO-Parameterserver → DEA00004*/ 

//vllt statt interface ne device-role übergeben (is ja n struct)
uu_id::uu_id(device_role * role):
		field2{0x6C, 0x97}     ,     
		field3{0x11, 0xD1},
		field4{0x82, 0x71, 0x00, 0xA0, 0x24, 0x42, 0xDF, 0x7D}
{
	field1[0]=0xDE;
	field1[1]=0xA0;
	field1[2]=0x00;
	if (*role==0x01) field1[3]=0x01; //IO-Device
	else if (*role==0x02) field1[3]=0x02;  //IO-Controller
	else if (*role==0x08) field1[3]=0x03;  //IO-Supervisor
	//parameterserver?? device_role kann auch multidevice sein (0x04) -> was dann??
}

unsigned char *  rpc_Header::toBuffer(){
	unsigned char* buffer = (unsigned char*)malloc(80);
	
	buffer[0]=version;
	buffer[1]=packetType;
	buffer[2]=flags1;
	buffer[3]=flags2;
	buffer[4]=dRep[0];
	buffer[5]=dRep[1];
	buffer[6]=dRep[2];
	buffer[7]=serialHigh;
	//8 bis 23 : objectUUID: memcpy??
	//TODO
	//24 bis 39 : interfaceUUID
	//TODO
	//40 bis 55: activityUUID
	//TODO
	//vorerst nur 0en:
	for (int i = 8; i<55; i++){
		buffer[i]=0x44;
	}
	//56-59 serverBootTime, 60-63 interfaceVersion, 64-67 sequenceNumber
	for (int i=0; i<4; i++){
		buffer[56+i]=serverBootTime[i];
		buffer[60+i]=interfaceVersion[i];
		buffer[64+i]=sequenceNumber[i];
	}
	
	//68-69 operationNumber, 70-71 interfaceHint, 72-73 activityHint, 74-75 lengthOfBody, 76-77 fragmentNumber) 
	for (int i=0; i<2; i++){
		buffer[68+i]=operationNumber[i];
		buffer[70+i]=interfaceHint[i];
		buffer[72+i]=activityHint[i];
		buffer[74+i]=lengthOfBody[i];
		buffer[76+i]=fragmentNumber[i];
	}
	
	buffer[78]=authentificationProtocoll;
	buffer[79]=serialLow;
	//
	return buffer;
}



void rpc_Header::construct(){
		version=0x04;
		flags1 = 0b00000100; //little&big endian?
		flags2 = 0b00000000; //bit 1 -> "abbruch lag am aufrufende vor" ???? was? 0 oder 1 jetzt??
		dRep[0]=0b11111111; //Encoding ENBCD & Little Endian
		dRep[1]=0x00; //IEEE Floating Point
		dRep[2]=0x00; //keine Bedeutung, sollen 0 sein
		
		memcpy(&sequenceNumber, &headerCount, 4*sizeof(u_char));
		headerCount++;
		
		interfaceHint[0] = 0xFF;
		interfaceHint[1] = 0xFF;
		activityHint[0] = 0xFF;
		activityHint[1] = 0xFF;
		//default: keine Fragmentierung
		fragmentNumber[0]=0x00;
		fragmentNumber[1]=0x00;
		u_char authenticationProtocoll=0; //0=keine Authentifikation
		
		
		
		//TODO: andere Felder setzen
	}

rpc_Header::rpc_Header(uu_id * oUUID, uu_id * iUUID, uu_id * aUUID){
		construct();
		objectUUID=oUUID;
		interfaceUUID=iUUID;
		activityUUID=aUUID;
		//hier u_char <-> int probleme zu erwarten,  bei ausgabe static_cast <unsigned> ??
		//allgemein zu testzwecken noch suboptimal umgesetzt
	}
	
		
rpc_Header::rpc_Header(){
		construct();
	}