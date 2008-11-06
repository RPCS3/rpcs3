#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <stdio.h>

#include <list>

struct pending_icmp_request
{
	char  ipaddress[4];
	HANDLE hEvent;
	DWORD  sTick;
	DWORD  timeOut; //ttl in ms
	DWORD  replyBufferSize;
	char  *replyBuffer;
	char  *requestData;
	void  *userdata;

	pending_icmp_request()
	{
		memset(this,0,sizeof(pending_icmp_request));
	}

	pending_icmp_request(pending_icmp_request&p)
	{
		memcpy(this,&p,sizeof(p));
	}
};

typedef std::list<pending_icmp_request> request_list;

request_list ping_list;

HANDLE hIP;

int icmp_init()
{
    hIP = IcmpCreateFile();

	if(hIP==INVALID_HANDLE_VALUE)
		return -1;

	return 0;
}

void icmp_start(unsigned char *ipaddr, int ttl, void *data, int datasize, void *udata)
{
	pending_icmp_request req;

	req.hEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	req.sTick = GetTickCount();
	req.timeOut = ttl;

	req.requestData = (char*)malloc(datasize);
	memcpy(req.requestData,data,datasize);

	memcpy(req.ipaddress,ipaddr,4);

	req.replyBufferSize = (sizeof(ICMP_ECHO_REPLY) + sizeof(datasize));
	req.replyBuffer = (char*)malloc(replyBufferSize);

	req.userdata=udata;

	ping_list.push_back(req);

	IcmpSendEcho2(hIP,req.hEvent,NULL,NULL,*(DWORD*)ipaddr,req.requestData,58,
					NULL,req.replyBuffer,replyBufferSize,ttl);
  
}

int icmp_check_replies(char *ipaddress, void **udata)
{
	for(request_list::iterator rit=ping_list.begin();rit!=ping_list.end();rit++)
	{
		if(WaitForSingleObject(rit->hEvent,0)==0) //handle is signaled, reply received.
		{
			if(IcmpParseReplies(rit->replyBuffer,rit->replyBufferSize)>0)
			{
				memcpy(ipaddress,rit->ipaddress,4);

				ping_list.remove(rit);

				return 1; //reply received
			}
			ResetEvent(rit->hEvent);
		}
		if(GetTickCount() >= (rit->sTick+rit->timeOut))
		{
			memcpy(ipaddress,rit->ipaddress,4);
			*udata = rit->userdata;

			ping_list.remove(rit);

			return 2; //timeout
		}
	}
	return 0;
}

void icmp_close()
{
    IcmpCloseHandle(hIP);
}