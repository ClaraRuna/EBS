#include "header.h"

#include <iostream>
#include <string.h> 
#include <netinet/in.h>
#include <net/if.h>

//#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <netinet/in.h>

#define RPC_HEADER_LENGTH 80
#define NRDDATA_LENGTH 20
#define DEFAULT_IF	"enx9cebe808bd01"


//initialize static variable with 0;
long rpc_Header::headerCount = 0; 
short IODHeader::SeqNumberCount = 0;
short ARBlockRequest::sessionKeyCounter = 0;

char etherName[IFNAMSIZ]; //Name of Ethernetport

void setEtherName(char *argv[]) {
	if(argv != NULL) {
		strcpy(etherName, argv[1]);
	} else {
		strcpy(etherName, DEFAULT_IF);
	}
}

//default: activity, beliebig, dient nur identifikation
uu_id::uu_id():
		field1{0x42, 0x42,0x42,0x42},
		field2{0x42, 0x42}     ,     
		field3{0x42, 0x42},
		field4{0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42}
{
}

//für ArUUID implizit i=0
uu_id::uu_id(u_char i):
		field1{i, i, i, i},
		field2{i,i},     
		field3{i,i},
		field4{i,i,i,i,i,i,i,i}
{
}

uu_id::uu_id(device*  object):
		field1{0xDE, 0xA0, 0x00, 0x00},
		field2{0x6C, 0x97},     
		field3{0x11, 0xD1},
		field4{0x82, 0x71, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
{
	field4[2] = 0x00; 	//höherwertiges byte der Instanz -> 0x00 bei IO-Divices/-Controler
	if(object->devRole == 0x02) { //IO-Controler
		field4[3] = 0x64;	//niederwertiges byte der Instanz 
	} else {	//IO-Device (oder Supervisor)
		field4[3] = 0x01;	//niederwertiges byte der Instanz 
	}
	
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


unsigned char * uu_id::toBuffer(){
	unsigned char * buffer= (unsigned char*)malloc(16);
	
	for (int i=0; i < 4; i++){
		buffer[i]=field1[i];
	//	std::cout << buffer[i] << " " ;
	}
	for (int i=0; i<2;i++){
		buffer[i+4]=field2[i];
		buffer[i+6]=field3[i];
	}

	for (int i=0; i<8; i++){
		buffer[i+8]=field4[i];
	}

	return buffer;
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
	memcpy(&buffer[8], objectUUID->toBuffer(), 16*sizeof(u_char));
	//24 bis 39 : interfaceUUID
	memcpy(&buffer[24], interfaceUUID->toBuffer(), 16*sizeof(u_char));
	//40 bis 55: activityUUID
	memcpy(&buffer[40], activityUUID->toBuffer(), 16*sizeof(u_char));
	//vorerst nur 0en:
	
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
	return buffer;
}

void rpc_Header::construct(){
	version = 0x04;
	packetType=0x00;
	flags1  = 0b00100000; //little-, & big-endian? --> geraten!
	flags2  = 0b00000000; //bit 1 -> "abbruch lag am aufrufende vor" ???? was? 0 oder 1 jetzt??
	dRep[0] = 0b00000000;
	//dRep[0] = 0b11110000; //Encoding ASCII & Little Endian
	dRep[1] = 0x00; //IEEE Floating Point
	dRep[2] = 0x00; //keine Bedeutung, sollen 0 sein
	serialHigh = 0x00; //das höherwertige Byte der Fragmentnummer des Aufrufs
	//ObjectUUID
	//InterfaceUUID
	//ActivityUUID
	serverBootTime[0] = 0x00;
	serverBootTime[1] = 0x00;
	serverBootTime[2] = 0x00;
	serverBootTime[3] = 0x00;
	interfaceVersion[0] = 0x00;
	interfaceVersion[1] = 0x00;
	interfaceVersion[2] = 0x00;
	interfaceVersion[3] = 0x01;

	long tempHeaderCount=htonl(headerCount); //needed for changing byte order
	memcpy(&sequenceNumber, &tempHeaderCount, 4*sizeof(u_char)); //sequenceNumber
	headerCount++;
	//little/big endian??
	operationNumber[0] = 0x00;
	operationNumber[1] = 0x05;
	
	interfaceHint[0] = 0xFF;
	interfaceHint[1] = 0xFF;
	activityHint[0] = 0xFF;
	activityHint[1] = 0xFF;
	//little/big endian??
	lengthOfBody[0] = 0x00;
	lengthOfBody[1] = 0x54;
	fragmentNumber[0] = 0x00; //default: keine Fragmentierung
	fragmentNumber[1] = 0x00;
	authentificationProtocoll = 0x00; //0 = keine Authentifikation
	serialLow = 0x00; //das niederwertige Byte der Fragmentnummer des Aufrufs
}

rpc_Header::rpc_Header(uu_id * oUUID, uu_id * iUUID, uu_id * aUUID){
	construct();
	objectUUID = oUUID;
	interfaceUUID = iUUID;
	activityUUID = aUUID;
	//hier u_char <-> int probleme zu erwarten,  bei ausgabe static_cast <unsigned> ??
	//allgemein zu testzwecken noch suboptimal umgesetzt
}
	
		
rpc_Header::rpc_Header(){
	construct();
}

NRDData::NRDData(){
	//Standard:Request
	for (int i = 0 ; i < 4 ; i++) {
		ArgsLength[i] = 0x00;
		Offset[i] = 0x00;
		ActualCount[i] = 0x00;
	}
	ArgsMaxStat[0] = MaxCount[0] = 0x00;
	ArgsMaxStat[1] = MaxCount[1] = 0x00;
	ArgsMaxStat[2] = MaxCount[2] = 0x02;
	ArgsMaxStat[3] = MaxCount[3] = 0x51;
	
	ArgsLength[3] = 0x40;
	ActualCount[3] = 0x40;
}

NRDData::NRDData(unsigned char maxLength[4], int bodyLength) {
	for (int i = 0 ; i < 4 ; i++) {
		ArgsMaxStat[i] = MaxCount[i] = maxLength[i];
		Offset[i] = 0x00;
	}
	ArgsLength[0] = ActualCount[0] = bodyLength / 16777216;
	ArgsLength[1] = ActualCount[1] = bodyLength / 65536;
	ArgsLength[2] = ActualCount[2] = bodyLength / 256;
	ArgsLength[3] = ActualCount[3] = bodyLength % 256;
}

unsigned char *  NRDData::toBuffer(){
	unsigned char* buffer = (unsigned char*)malloc(sizeof(NRDData));
	for (int i = 0; i < 4; i++){
		buffer[i] 	 = ArgsMaxStat[i];
		buffer[4+i]  = ArgsLength[i];
		buffer[8+i]  = MaxCount[i];
		buffer[12+i] = Offset[i];
		buffer[16+i] = ActualCount[i];
	}
	return buffer;
}

BlockHeader::BlockHeader(){
	
	//little/big endian?
	BlockType[0] = 0x00;
	BlockType[1] = 0x09; 	//0x20;
	//little/big endian?
	//wahrscheinlich 2 bei request
	BlockLength[0] = 0x00;  	//60, länge des Rest-IOD-Headers
	BlockLength[1] = 0x3c;
	BlockVersionHigh = 0x01;
	BlockVersionLow = 0x00;		//in manchen fällen auch 1/0
	
}

unsigned char *  BlockHeader::toBuffer(){
	unsigned char* buffer = (unsigned char*)malloc(sizeof(BlockHeader));
	buffer[0] = BlockType[0];
	buffer[1] = BlockType[1];
	buffer[2] = BlockLength[0];
	buffer[3] = BlockLength[1];
	buffer[4] = BlockVersionHigh;
	buffer[5] = BlockVersionLow;
	
	return buffer;
}

IODHeader::IODHeader(BlockHeader * bHeader){
	blockHeader = bHeader;
	memcpy(&SeqNumber, &SeqNumberCount, 2*sizeof(u_char)); //sequenceNumber
	SeqNumberCount++;
	ArUUID= * new uu_id (static_cast <u_char> (0)) ; //0 weil implizite verbindung
	for (int i = 0; i < 4; i++) {
		API[i] = 0x00;
	}
	//bei i&m0 anfrage slot und subslot nicht ausgewertet
	for (int i = 0; i < 2; i++) {
		Slot[i] = 0x00;
		Subslot[i] = 0x00;
		Padding1[i] = 0x00;
	}
	//Indexnr für I&M0-Filterdaten auslesen
	
	//little/big endian
	Index[0] = 0xF8;
	Index[1] = 0x40;
	for (int i = 0; i < 4; i++) {
		DataLength[i] = 0;
	}
	DataLength[2] = 0x10; 
	
	
	targetArUUID=*new uu_id(static_cast <u_char>(0));
	for (int i = 0; i < 8; i++) {
		Padding2[i]=0x00;
	}
}

unsigned char *  IODHeader::toBuffer() {
	unsigned char* buffer = (unsigned char*)malloc(sizeof(IODHeader));
	
	unsigned char* bHeader = blockHeader->toBuffer();
	memcpy(&buffer[0], bHeader, 6*sizeof(u_char)); 
	
	//little/big endian
	buffer[6] = SeqNumber[0];
	buffer[7] = SeqNumber[1];
	
	unsigned char* arID = ArUUID.toBuffer();
	memcpy(&buffer[8], arID, 16*sizeof(u_char));
	
	for (int i=0; i<4; i++){
		buffer[24+i] = API[i];
	}
	
	for (int i=0 ; i<2; i++){
		buffer[28+i] = Slot[i];
		buffer[30+i] = Subslot[i];
		buffer[32+i] = Padding1[i];
		buffer[34+i] = Index[i];
	}
	for (int i=0; i<4; i++){
		buffer[36+i] = DataLength[i];
	}
	
	unsigned char* targetID = targetArUUID.toBuffer();
	memcpy(&buffer[40], targetID, 16*sizeof(u_char));
	
	for (int i=0; i<8; i++){
		buffer[56+i]=Padding2[i];
	}
	
	return buffer;
}

ARBlockRequest::ARBlockRequest(){
	ARType[0] = 0x00;
	ARType[1] = 0x06;
	ArUUID = new uu_id(20);		//20 -> beliebige zahl, die die relation identifiziert
	
	sessionKey[0] = sessionKeyCounter / 256;
	sessionKey[1] = sessionKeyCounter & 256;
	sessionKeyCounter++;
	
	struct ifreq if_mac;
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, etherName, IFNAMSIZ-1);
	if (ioctl(4, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	
	CMInitiatorMAC[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	CMInitiatorMAC[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	CMInitiatorMAC[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	CMInitiatorMAC[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	CMInitiatorMAC[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	CMInitiatorMAC[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	
	CMInitiatorObjectUUID = new uu_id (new device());		//random leeres device, damit wir ne objectuuid haben
	CMInitiatorObjectUUID->field4[2] = 0x00;			//instanz
	CMInitiatorObjectUUID->field4[3] = 0x01;			
	CMInitiatorObjectUUID->field4[4] = 0x01;			//DevID
	CMInitiatorObjectUUID->field4[5] = 0x03;
	CMInitiatorObjectUUID->field4[6] = 0x01;			//vendorID
	CMInitiatorObjectUUID->field4[7] = 0x17;			
	
	ArProperties [0] = 0x00;
	ArProperties [1] = 0x00;
	ArProperties [2] = 0x01;
	ArProperties [3] = 0x31;
	
	CMInitiatorActivityTimeoutFactor[0] = 0x02;
	CMInitiatorActivityTimeoutFactor[1] = 0x58;		//sollten 60sek sein (faktor 100ms)
	
	InitiatorUDPRTPort[0] = 0x88;
	InitiatorUDPRTPort[1] = 0x92;
	
	short NameLength = sizeof(etherName) - 1;
	std::cout << "Testnamenlänge: " << sizeof(etherName) << std::endl;
	StationNameLength[0] = NameLength / 256;
	StationNameLength[1] = NameLength % 256;
	
	//BlockHeader
	blockHeader = new BlockHeader();
	blockHeader->BlockType[0]=0x01;
	blockHeader->BlockType[1]=0x01;
	
	short size = 54 + NameLength;		//ARBlockRequest -4 + stationNamelength
	blockHeader->BlockLength[0] = size / 256; 
	blockHeader->BlockLength[1] = size % 256;

}

unsigned char *  ARBlockRequest::toBuffer(){
	unsigned char* buffer = (unsigned char*)malloc(sizeof(ARBlockRequest) + StationName.size());
	
	unsigned char* bHeader = blockHeader->toBuffer();
	
	memcpy(&buffer[0], bHeader, 6*sizeof(u_char));

	buffer[6] = ARType[0];
	buffer[7] = ARType[1];
	
	unsigned char* arID = ArUUID->toBuffer();
	memcpy(&buffer[8], arID, 16*sizeof(u_char));
	
	buffer[24]=sessionKey[0];
	buffer[25]=sessionKey[1];
	
	for (int i =0; i<6; i++){
		buffer[26+i]=CMInitiatorMAC[i];
	}
	
	unsigned char* cmInitiatorObjectUUID = CMInitiatorObjectUUID->toBuffer();
	memcpy(&buffer[32], cmInitiatorObjectUUID, 16*sizeof(u_char));
	
	for (int i = 0 ; i<4; i++){
		buffer[48+i] = ArProperties[i];
	}
	
	buffer[52] = CMInitiatorActivityTimeoutFactor[0];
	buffer[53] = CMInitiatorActivityTimeoutFactor[1];
	
	buffer[54] = InitiatorUDPRTPort[0];
	buffer[55] = InitiatorUDPRTPort[1];
	
	buffer[56] = StationNameLength[0];
	buffer[57] = StationNameLength[1];
	
	std::cout << etherName << std::endl;
	std::cout << StationNameLength << std::endl;

	for (int i = 0; i < sizeof(etherName); i++){
		buffer[58+i] = etherName[i];
	}

	return buffer;
}
