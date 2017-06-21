#include <vector>
#include <iomanip>

enum device_role{
	ioDevice = 0x01,
	ioController = 0x02,
	ioMultidevice = 0x04,
	ioSupervisor = 0x08, 
	nichtGesetzt = 0xF0
};

typedef struct option {
	//operator<<(std::ostream&, device_role const&)
	int opt;
	int subopt;
}OPT;

typedef struct ip_param{
	//vollkommen idiotisch, dass als int zu speichern, aber mit u_char geht's nicht, obwohl es theoretisch ja genauso gehen müsste wie bei d vendor_id und MAC
	u_char ip [4];
	u_char subnet[4];
	u_char gateway[4];
	
	ip_param(){
	for (int i = 0 ; i<4 ; i++){
		ip[i]=0;
		subnet[i]=0;
		gateway[i]=0;
	}
	}
}IPPAR;

typedef struct device {
	u_char MAC[6];
	std::string name;
	std::string vendor;
	u_char vendor_id [2];
	u_char device_id [2];
	device_role devRole;
	std::vector<option*> options;
	ip_param ipParam;
	
	//konstruktor
	device(){
	for (int i = 0; i<6; ++i){
		MAC[i]=0;
	}
	for (int i = 0; i<2; ++i){
		vendor_id[i]='\0';
		device_id[i]='\0';
	}
	devRole=static_cast<device_role>(0xF0);
	}
}DEV;

typedef struct dcp_data_header{
	u_char opt;
	u_char subopt;
	unsigned short length;
}DCPDH;

typedef struct dcpHeader {
	unsigned char serviceID;
	unsigned char serviceType;
	unsigned char xID[4];
	unsigned char respDelayFactor[2];
	unsigned char dcpDataLength[2];

	//Constructor
	dcpHeader(	unsigned char serviceID,
				unsigned char serviceType,
				unsigned char xID[4],
				unsigned char respDelayFactor[2],
				unsigned char dcpDataLength[2]) {
		this->serviceID = serviceID;
		this->serviceType = serviceType;
		for(int i=0; i<4; i++) {
			this->xID[i] = xID[i];
			if(i<2) {
				this->respDelayFactor[i] = respDelayFactor[i];
				this->dcpDataLength[i] = dcpDataLength[i];		
			}
		}
	}
}dcpHeader;

typedef struct dcpDataHeader {
	unsigned char option;
	unsigned char subOption;
	unsigned char length[2];
	unsigned char status[2];

	//Constructor
	dcpDataHeader(	unsigned char option,
					unsigned char subOption,
					unsigned char length[2],
					unsigned char status[2]) {
		this->option = option;
		this->subOption = subOption;
		for(int i=0; i<2; i++) {
			this->length[i] = length[i];
			this->status[i] = status[i];
		}
	}
}dcpDataHeader;

/***********************************************************
************RCP STRUCTS*************************************
***********************************************************/

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
	long get_headerCount();
	
}RPCHEADER;

