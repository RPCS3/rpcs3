/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "net.h"
#include "..\Dev9.h"

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
	emu_printf("Waiting for RX-net thread to terminate..");
	WaitForSingleObject(rx_thread,-1);
	emu_printf(".done\n");

	delete nif;
}