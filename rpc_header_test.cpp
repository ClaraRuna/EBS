#include "rpc.h"
#define RPC_HEADER_LENGTH 80
#define UUID_LENGHT 16



int main(){
	uu_id * test = new uu_id;
	unsigned char* buffer = test->toBuffer();
	
	for (int i=0; i<UUID_LENGHT; i++){
		std::cout<< static_cast<int>(buffer[i])<< " ";
		if (i%10==0) std::cout << "            ";
	}
	
	
	rpc_Header * rpcHeader=new rpc_Header(test, test, test);

	
	for (int i = 0; i< RPC_HEADER_LENGTH; i++){
		std::cout <<static_cast<int> (buffer[i])<<" ";
		if (i%10==0) std::cout<<"               ";
		if (i%20==0) std::cout<<std::endl;
	}
	
	
	std::cout<<std::endl;
}