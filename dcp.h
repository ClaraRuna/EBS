#include <iostream>
#include <vector>
#include <iomanip>

enum device_role{
	ioDevice = 0x01,
	ioController = 0x02,
	ioMultidevice = 0x04,
	ioSupervisor = 0x08, 
	nichtGesetzt = 0xF0
};

std::ostream& operator << (std::ostream& os, const device_role& role){
	//os << role << ": ";
	switch (role){
		case 0x01: os<<"IO-Device"; break;
		case 0x02: os<<"IO-Controller"; break;
		case 0x04: os<<"IO-Multidevice"; break;
		case 0x08: os<<"IO-Supervisor"; break;
		case 0xF0: os<<"nicht gesetzt"; break;
		default : os<<"ungültige nr: " << static_cast <int> (role); break; 
	}
	return os;
}

struct option{
	int opt;
	int subopt;
} OPT;

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


std::ostream& operator << (std::ostream& os, const dcp_data_header& header){
	return os << " opt: " << static_cast <unsigned> (header.opt) //<< std::endl
		<< " subopt: " << static_cast <unsigned> (header.subopt) //<< std::endl
		<< " length: " << header.length << std::endl;
}


std::ostream& operator << (std::ostream& os, unsigned char* buffer){
	int temp;
	int data_length=0;
	os << std::hex;
	os <<"Ethernet Header: \n"
		<< "   Destination Mac:";
		for (temp=0; temp<6; temp++){
			os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
		}
		os << "\n   Source Mac: ";
		for (temp=6;temp<12; temp++){
			os <<std::setw (2) << std::setfill('0')<<static_cast <unsigned> (buffer [temp]) << ":";
		}
		os << "\n   Ether Type: ";
		for (temp=12; temp<14; temp++){
			os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
		}
		os << "\n   Frame ID: " ;
		for (temp=14; temp<16; temp++){
			os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
		}
		os << " signals ";
		
		//alles weitere nur, wenn bekannter pakettyp
		if (buffer [14]==0xFE && (buffer[15]==0xFD || buffer[15]==0xFC||buffer[15]==0xFE||buffer[15]==0xFF) ){
			switch(buffer[15]){
					case 0xFC: os<<"Hello Request"; break;
					case 0xFD: os<<"Set- bzw Get- Request und Response"; break;
					case 0xFE: os<<"Identify Request"; break;
					case 0xFF: os<<"Identify Response"; break;
					default: os<<"that this program is buggy"; break;
				}	
			//weiteren stuff verarbeiten (DCP-Header und DCP-Pakete
			os << "\n DCP-Header: \n" <<
				"   Service Id: " <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [16]);
			os << "\n   Service Type: " <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [17]);
			os << "\n   Xid: ";
			for (temp=18; temp<22; temp++){
				os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
			}
			os << "\n   Response Delay Factor: ";
			for (temp=22; temp<24; temp++){
				os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
			}
			os << "\n   DCP-Data-Length: ";
			for (temp=24; temp<26; temp++){
				os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [temp]) << ":";
			}
			os<<"\n";
			//set total length of packet
			data_length=buffer[25]+buffer[24]*256;
			
			temp=26;
			while (temp<data_length+25){
				struct dcp_data_header dcp_dh;
				dcp_dh.opt=buffer[temp];
				dcp_dh.subopt=buffer[temp+1];
				dcp_dh.length=buffer[(temp+2)]*256+buffer[temp+3];
				
				//print dataheader
				std::cout <<"   "<< dcp_dh;
				
				os << "      DCP-Data: ";
				for (int i=temp+4; i<temp+4+dcp_dh.length;i++){
					os <<std::setw (2) << std::setfill('0')<< static_cast <unsigned> (buffer [i]) << ":";
				}
				os << "\n";
				
				temp=temp+4+dcp_dh.length;
				//fill padding
				if (dcp_dh.length % 2 ==1) temp+=1;
		}
			
		}
		else os << "someting unknown\n";
		os<<"\n\n"; 
	return os;
}

std::ostream& operator << (std::ostream& os, const std::vector<option*>& o){
 	for (int i=0; i< o.size(); i++){
		os <<  std::dec << (i+1) << std::hex << ".[" << static_cast <unsigned> (o[i]->opt) << ";" << static_cast <unsigned>(o[i]->subopt) << "] ";	
	}
	if(o.size() == 0) {
		os << "no options";
	}
	os << std::endl;
}

std::ostream& operator << (std::ostream& os, const device& dev){
	os << "mac: " << std::hex << std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[0])<<":"
				<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[1])<<":"
				<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[2])<<":"
				<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[3])<<":"
				<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[4])<<":"
				<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.MAC[5]) <<std::endl
			<< "stationsname: " << dev.name << std::endl
			<< "vendor: " << dev.vendor << std::endl
			<< "vendor_id: " << std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.vendor_id[0])<<":"
					<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.vendor_id[1])<<std::endl
			<< "device_id: " << std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.device_id[0])<<":"
					<< std::setw(2) << std::setfill('0') << static_cast <unsigned> (dev.device_id[1])<<std::endl
			<<"device_role: "<<dev.devRole <<std::endl
			<<"supported options: " << dev.options;
	if (!(dev.ipParam.ip[0]==0 && dev.ipParam.ip[1]==0 && dev.ipParam.ip[2]==0 && dev.ipParam.ip[3]==0)){
		os << "ip: " << static_cast <unsigned>( dev.ipParam.ip[0] )<<"."<< static_cast <unsigned>(dev.ipParam.ip[1]) <<"."<< static_cast <unsigned>( dev.ipParam.ip[2]) <<"."<<  static_cast <unsigned>(dev.ipParam.ip[3])<< std:: endl
			<<"Subnet: " << static_cast <unsigned>(dev.ipParam.subnet[0]) <<"."<< static_cast <unsigned>(dev.ipParam.subnet[1]) <<"."<<  static_cast <unsigned>(dev.ipParam.subnet[2]) <<"."<<  static_cast <unsigned>(dev.ipParam.subnet[3])<< std:: endl
			<<"Gateway: " << static_cast <unsigned>(dev.ipParam.gateway[0]) <<"."<< static_cast <unsigned>(dev.ipParam.gateway[1]) <<"."<<static_cast <unsigned>(  dev.ipParam.gateway[2]) <<"."<<  static_cast <unsigned>(dev.ipParam.gateway[3])<< std:: endl;			
	}
	return os; 
}

