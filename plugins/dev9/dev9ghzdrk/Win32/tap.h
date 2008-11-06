#pragma once
#include <vector>
#include "net.h"
using namespace std;

struct tap_adapter
{
	string name;
	string guid;
};
vector<tap_adapter>* GetTapAdapters();
class TAPAdapter : public NetAdapter
{
	HANDLE htap;
	OVERLAPPED read,write;
public:
	TAPAdapter();
	virtual bool blocks();
	//gets a packet.rv :true success
	virtual bool recv(NetPacket* pkt);
	//sends the packet and deletes it when done (if successful).rv :true success
	virtual bool send(NetPacket* pkt);
	virtual ~TAPAdapter();
};