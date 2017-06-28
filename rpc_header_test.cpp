#include "struct.h"
#include <iostream>
#define RPC_HEADER_LENGTH 80
#define UUID_LENGTH 16



int main(){
	uu_id * test = new uu_id;
	
	/*unsigned char* buffer = test->toBuffer();
	
	for (int i=0; i<UUID_LENGTH; i++){
		std::cout<< static_cast<int>(buffer[i])<< " ";
	}*/

	rpc_Header * rpcHeader=new rpc_Header(test, test, test);

	unsigned char* buffer = rpcHeader->toBuffer();
	
	for (int i = 0; i< RPC_HEADER_LENGTH; i++){
		std::cout <<static_cast<int> (buffer[i])<<" ";
		if ((i+1)%10==0) std::cout << "// ";
	}
	
	
	std::cout<<std::endl;
}