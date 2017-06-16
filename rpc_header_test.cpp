#include "rpc.h"

int main(){
	struct rpc_Header rpcHeader;
	unsigned char* buffer = rpcHeader.toBuffer();
	
	for (int i = 0; i< MAX_BUFFER; i++){
		std::cout <<static_cast<int> (buffer[i])<<" ";
	}
	std::cout<<std::endl;
}