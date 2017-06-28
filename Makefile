test: task3.cpp recieve.cpp sendDCP.cpp sendUDP.cpp header.h struct.h outstream.h rpc.cpp
	g++ -std=c++11 -pthread -o test task3.cpp recieve.cpp sendDCP.cpp sendUDP.cpp rpc.cpp
	