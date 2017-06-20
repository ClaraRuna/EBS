#include "rpc.h"
#define RPC_HEADER_LENGTH 80



int main(){
	rpc_Header * rpcHeader=new rpc_Header;
	unsigned char* buffer = rpcHeader->toBuffer();
	
	for (int i = 0; i< RPC_HEADER_LENGTH; i++){
		std::cout <<static_cast<int> (buffer[i])<<" ";
		if (i%10==0) std::cout<<"               ";
		if (i%20==0) std::cout<<std::endl;
	}
	
	rpc_Header * rpcHeader1=new rpc_Header;
	rpc_Header * rpcHeader2=new rpc_Header;
	rpc_Header * rpcHeader3=new rpc_Header;
	
	buffer = rpcHeader->toBuffer();
	
	for (int i = 0; i< RPC_HEADER_LENGTH; i++){
		std::cout <<static_cast<int> (buffer[i])<<" ";
		if (i%10==0) std::cout<<"               ";
		if (i%20==0) std::cout<<std::endl;
	}
	
	
	std::cout<<std::endl;
}