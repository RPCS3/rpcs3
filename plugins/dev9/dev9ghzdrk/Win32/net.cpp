#include "net.h"
#include "Dev9.h"

//mtfifo<NetPacket*> rx_fifo;
//mtfifo<NetPacket*> tx_fifo;

NetAdapter* nif;
HANDLE rx_thread;

volatile bool RxRunning=false;
//rx thread
DWORD WINAPI NetRxThread(LPVOID lpThreadParameter)
{	
	NetPacket tmp;
	while(RxRunning)
	{
		while(rx_fifo_can_rx() && nif->recv(&tmp))
		{
			rx_process(&tmp);
		}
		
		Sleep(10);
	}

	return 0;
}

void tx_put(NetPacket* pkt)
{
	nif->send(pkt);
	//pkt must be copied if its not processed by here, since it can be allocated on the callers stack
}
void InitNet(NetAdapter* ad)
{
	nif=ad;
	RxRunning=true;

	rx_thread=CreateThread(0,0,NetRxThread,0,CREATE_SUSPENDED,0);

	SetThreadPriority(rx_thread,THREAD_PRIORITY_HIGHEST);
	ResumeThread(rx_thread);
}
void TermNet()
{
	RxRunning=false;
	printf("Waiting for RX-net thread to terminate..");
	WaitForSingleObject(rx_thread,-1);
	printf(".done\n");

	delete nif;
}