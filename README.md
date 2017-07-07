#EBS

compile with 
g++ task2.cpp -std=c++11 -pthread -o task2.out

run with 
sudo ./task2.out [nameOfEthernet]

filtern wireshark
	DCP:
		eth.addr==b8:88:e3:3a:41:29 and (pn_io or dcerpc or pn_dcp)
		eth.addr==9c:eb:e8:08:bd:01 and (pn_io or dcerpc or pn_dcp)
	RPC:		
		ip.addr == 172.16.4.152 and !dns
		ip.addr == 172.16.4.160 and !dns

9c:eb:e8:08:bd:01
