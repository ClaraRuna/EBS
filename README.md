#EBS

compile with 
g++ task2.cpp -std=c++11 -pthread -o task2.out

run with 
sudo ./task2.out [nameOfEthernet]

filtern wireshark
ip.addr == 172.16.4.152 and !dns

eth.addr==b8:88:e3:3a:41:29 and (pn_io or dcerpc or pn_dcp)