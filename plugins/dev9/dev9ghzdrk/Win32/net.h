#pragma once
#include <stdlib.h>
#include <string.h>  //uh isnt memcpy @ stdlib ?

struct NetPacket
{
	NetPacket() {size=0;}
	NetPacket(void* ptr,int sz) {size=sz;memcpy(buffer,ptr,sz);}

	int size;
	char buffer[2048-sizeof(int)];//1536 is realy needed, just pad up to 2048 bytes :)
};
/*
extern mtfifo<NetPacket*> rx_fifo;
extern mtfifo<NetPacket*> tx_fifo;
*/

class NetAdapter
{
public:
	virtual bool blocks()=0;
	virtual bool recv(NetPacket* pkt)=0;	//gets a packet
	virtual bool send(NetPacket* pkt)=0;	//sends the packet and deletes it when done
	virtual ~NetAdapter(){}
};

void tx_put(NetPacket* ptr);
void InitNet(NetAdapter* adapter);
void TermNet();