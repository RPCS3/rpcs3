/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include "resource.h"
#include "Debugger.h"
#include "RDebugger.h"
#include "Common.h"
#include "PsxCommon.h"
#include "win32.h"
#include "../rdebug/deci2.h"

u32		port=8510;
SOCKET	serversocket, remote;
char	message[1024];		//message to add to listbox

int		runStatus=STOP, runCode=0, runCount=1;
HANDLE	runEvent=NULL;

DECI2_DBGP_BRK	ebrk[32],
				ibrk[32];
int				ebrk_count=0,
				ibrk_count=0;

int	debuggedCpu=0;	//default is to debug EE cpu;	IOP=1
u32 breakAddress=0;	//disabled; ie. you cannot use address 0 for a breakpoint
u32 breakCycle=0;	//disabled; ie. you cannot stop after 0 cycles

int CreateSocket(HWND hDlg, int port){
	WSADATA wsadata;
	SOCKADDR_IN saServer;
    
	if (WSAStartup( MAKEWORD(1, 1), &wsadata) ||
		((serversocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==INVALID_SOCKET)){
			MessageBox(hDlg, "Could not create socket\n[Is TCP/IP installed? WinSock 1.1 or above?]", 0, MB_OK);
			return FALSE;
	}
	sprintf(message, "[PCSX2] %s status=%s", wsadata.szDescription, wsadata.szSystemStatus);
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
	
	saServer.sin_family = AF_INET;
	saServer.sin_addr.S_un.S_addr = INADDR_ANY;	// accept any address
	saServer.sin_port = htons(port);		// port to listen to
	
	if (bind(serversocket, (LPSOCKADDR)&saServer, sizeof(struct sockaddr))==SOCKET_ERROR){
		sprintf(message, "Could not bind to port %d\n"
			"[Is there another server running on that port?]", port);
		MessageBox(hDlg, message, 0, MB_OK);
        closesocket(serversocket);
		return FALSE;
	}
	sprintf(message, "[PCSX2] Port %d is opened", port);
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
	
	// SOMAXCONN connections in queque? maybe 1 is enough...
	if (listen(serversocket, SOMAXCONN) == SOCKET_ERROR){
		sprintf(message, "Listening for a connection failed\n"
			"[dunno?]");
		MessageBox(hDlg, message, 0, MB_OK);
        closesocket(serversocket);
		return FALSE;
	}
	sprintf(message, "[PCSX2] Listening for a connection to establish...");
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
	
	cpuRegs.CP0.n.EPC=cpuRegs.pc;
	psxRegs.CP0.n.EPC=psxRegs.pc;
	sprintf(message, "%08X", cpuRegs.pc);
	Edit_SetText(GetDlgItem(hDlg, IDC_EEPC), message);
	sprintf(message, "%08X", psxRegs.pc);
	Edit_SetText(GetDlgItem(hDlg, IDC_IOPPC), message);
	sprintf(message, "%d", cpuRegs.cycle);
	Edit_SetText(GetDlgItem(hDlg, IDC_EECYCLE), message);
	sprintf(message, "%d", psxRegs.cycle);
	Edit_SetText(GetDlgItem(hDlg, IDC_IOPCYCLE), message);
	Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUGEE),  (debuggedCpu==0));
	Button_SetCheck(GetDlgItem(hDlg, IDC_DEBUGIOP), (debuggedCpu==1));
	Edit_LimitText(GetDlgItem(hDlg, IDC_STOPAT), 8);	//8 hex digits
	Edit_LimitText(GetDlgItem(hDlg, IDC_STOPAFTER), 10);//10 decimal digits
	sprintf(message, "%08X", breakAddress);
	Edit_SetText(GetDlgItem(hDlg, IDC_STOPAT), message);
	sprintf(message, "%d", breakCycle);
	Edit_SetText(GetDlgItem(hDlg, IDC_STOPAFTER), message);
	
	Button_Enable(GetDlgItem(hDlg, IDC_DEBUGIOP), FALSE);////////////////////////

	return TRUE;
}

int readData(char *buffer){
	int r, count=0;
	u8	*p=buffer;

	memset(buffer, 0, BUFFERSIZE);
	while (((count+= r = recv(remote, p, BUFFERSIZE, 0))!=INVALID_SOCKET) &&
		(count<*(u16*)buffer))
			p+=r;
	
	if (r==INVALID_SOCKET)
		return 0;
	
	return count;
}

int writeData(char *result){
	int r;/*, i;
	static char l[300], p[10];
	DECI2_HEADER *header=(DECI2_HEADER*)result;
*/
	r = send(remote, result, *(u16*)result, 0);
	if (r==SOCKET_ERROR)
        return 0;
/*
	sprintf(l, "size=%d, src=%c dst=%c proto=0x%04X ",
		header->length-8, header->source,	header->destination, header->protocol);
	for (i=8; i<*(u16*)result; i++){
		sprintf(p, "%02X ", result[i]);
		strcat(l, p);
	}
	SysMessage(l);
*/
	return r;
}

DWORD WINAPI ServingFunction(LPVOID lpParam){
	static u8	buffer[BUFFERSIZE],				//a big buffer
				result[BUFFERSIZE],				//a big buffer
				eepc[9], ioppc[9], eecy[15], iopcy[15];
	SOCKADDR_IN saClient;
	HWND hDlg=(HWND)lpParam;
	DWORD		size=sizeof(struct sockaddr);
	int exit=FALSE;
	
	if ((remote = accept(serversocket, (struct sockaddr*)&saClient, &size))==INVALID_SOCKET){
		ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), "[PCSX2] Commmunication lost. THE END");
		return FALSE;
	}
	sprintf(message, "[PCSX2] Connected to %d.%d.%d.%d on remote port %d",
		saClient.sin_addr.S_un.S_un_b.s_b1,
		saClient.sin_addr.S_un.S_un_b.s_b2,
		saClient.sin_addr.S_un.S_un_b.s_b3,
		saClient.sin_addr.S_un.S_un_b.s_b4,
		saClient.sin_port);
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), "[PCSX2] Start serving...");
	connected=1;//from this point on, all log stuff goes to ttyp
	
	//sendBREAK('E', 0, 0xff, 0x21, 1);	//do not enable this unless you know what you are doing!
	while (!exit && readData(buffer)){
		DECI2_HEADER *header=(DECI2_HEADER*)buffer;

		switch(header->protocol){
			case 0x0000:exit=TRUE;									break;
			case PROTO_DCMP:D2_DCMP(buffer, result, message);	break;
//			case 0x0120:D2_DRFP_EE(buffer, result, message);	break;
//			case 0x0121:D2_DRFP_IOP(buffer, result, message);	break;
//			case 0x0122:break;
			case PROTO_IDBGP:D2_DBGP(buffer, result, message, eepc, ioppc, eecy, iopcy);	break;
//			case 0x0140:break;
			case PROTO_ILOADP:D2_ILOADP(buffer, result, message);	break;
			case PROTO_EDBGP:D2_DBGP(buffer, result, message, eepc, ioppc, eecy, iopcy);break;
//			case 0x0240:break;
			case PROTO_NETMP:D2_NETMP(buffer, result, message);		break;
			default:
				sprintf(message, "[DECI2 %c->%c/%04X] Protocol=0x%04X",
					header->source, header->destination,
					header->length, header->protocol);
				ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
				continue;
		}
		if (exit==FALSE){
			ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), message);
			Edit_SetText(GetDlgItem(hDlg, IDC_EEPC), eepc);
			Edit_SetText(GetDlgItem(hDlg, IDC_IOPPC), ioppc);
			Edit_SetText(GetDlgItem(hDlg, IDC_EECYCLE), eecy);
			Edit_SetText(GetDlgItem(hDlg, IDC_IOPCYCLE), iopcy);
		}
	}
	connected=0;
	
	ListBox_AddString(GetDlgItem(hDlg, IDC_COMMUNICATION), "[PCSX2] Connection closed. THE END");
	return TRUE;
}

DWORD WINAPI Run2(LPVOID lpParam){
	HWND hDlg=(HWND)lpParam;
	static char pc[9];
	int i;
		
	while (1){
		if (runStatus==RUN){
			if (PSMu32(cpuRegs.pc)==0x0000000D){
				sendBREAK('E', 0, runCode, 0x22, runCount);
				InterlockedExchange(&runStatus, STOP);
				continue;
			}
			if ((runCode==2) && (//next
				((PSMu32(cpuRegs.pc) & 0xFC000000)==0x0C000000) ||//JAL
				((PSMu32(cpuRegs.pc) & 0xFC00003F)==0x00000009) ||//JALR
				((PSMu32(cpuRegs.pc) & 0xFC00003F)==0x0000000C)	  //SYSCALL
				)){u32 tmppc=cpuRegs.pc, skip=(PSMu32(cpuRegs.pc) & 0xFC00003F)==0x0000000C ? 4 : 8;
				while (cpuRegs.pc!=tmppc+skip)
					Cpu->Step();
			}else
				Cpu->Step();		//use this with breakpoints & step-by-step
//				Cpu->ExecuteBlock();	//use this to run faster, but not for stepping
//			sprintf(pc, "%08X", cpuRegs.pc);Edit_SetText(GetDlgItem(hDlg, IDC_EEPC), pc);
//			sprintf(pc, "%08X", psxRegs.pc);Edit_SetText(GetDlgItem(hDlg, IDC_IOPPC), pc);
//			sprintf(pc, "%d", cpuRegs.cycle);Edit_SetText(GetDlgItem(hDlg, IDC_EECYCLE), pc);
//			sprintf(pc, "%d", psxRegs.cycle);Edit_SetText(GetDlgItem(hDlg, IDC_IOPCYCLE), pc);
			if (runCount!=0 && --runCount==0){
				sendBREAK('E', 0, runCode, 0x23, runCount);
				InterlockedExchange(&runStatus, STOP);
				continue;
			}
			if ((breakAddress) && (breakAddress==cpuRegs.pc)){
				sendBREAK('E', 0, runCode, 0x21, runCount);
				InterlockedExchange(&runStatus, STOP);
				continue;			
			}
			if ((breakCycle) && (breakCycle==cpuRegs.cycle)){
                sendBREAK('E', 0, runCode, 0x21, runCount);
				InterlockedExchange(&runStatus, STOP);
				breakCycle=0;
				Edit_SetText(GetDlgItem(hDlg, IDC_STOPAFTER), "0");
				continue;			
			}
			for (i=0; i<ebrk_count; i++)
				if (cpuRegs.pc==ebrk[i].address && ebrk[i].count!=0){
					if (ebrk[i].count==0xFFFFFFFF || (--ebrk[i].count)==0){
						sendBREAK('E', 0, runCode, 0x22, runCount);
						InterlockedExchange(&runStatus, STOP);
					}
					break;
				}
		}
		else{
			cpuRegs.CP0.n.EPC=cpuRegs.pc;
			psxRegs.CP0.n.EPC=psxRegs.pc;
			ResetEvent(runEvent);
			WaitForSingleObject(runEvent, INFINITE);
			runStatus=RUN;
		}
	}

	return TRUE;		//never get here
}

LRESULT WINAPI RemoteDebuggerProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	static HANDLE	th=NULL, runth=NULL;
	static DWORD	thid=0;
	
	switch(uMsg){
		case WM_INITDIALOG:
			ShowCursor(TRUE);
			//open socket & thread
			ListBox_SetHorizontalExtent(GetDlgItem(hDlg, IDC_COMMUNICATION), 1700);
			if (CreateSocket(hDlg, port)==FALSE){
				ClosePlugins();
				EndDialog(hDlg, FALSE );
				return FALSE;
			}
			connected=0;
			th=CreateThread(NULL, 0, ServingFunction, (LPVOID)hDlg, 
				CREATE_SUSPENDED, &thid);
			runth=CreateThread(NULL, 0, Run2, (LPVOID)hDlg, 
				CREATE_SUSPENDED, &thid);
			runEvent=CreateEvent(NULL, TRUE, FALSE, "RunState");
			if (th==NULL || runth==NULL || runEvent==NULL){
				MessageBox(hDlg, _("Could not create threads or event"), 0, MB_OK);
				connected=0;
				closesocket(serversocket);
				ClosePlugins();
				EndDialog(hDlg, FALSE );
				return FALSE;
			}
			ResumeThread(runth);
			ResumeThread(th);//run, baby, run...
			return TRUE;

		case WM_COMMAND:
			if ((HIWORD (wParam) == EN_CHANGE) &&
				((LOWORD(wParam)==IDC_STOPAT)||(LOWORD(wParam)==IDC_STOPAFTER))){
					if (LOWORD(wParam)==IDC_STOPAT){char at[8+1]; int i;
						Edit_GetText((HWND)lParam, at, 8+1);
                        sscanf (at, "%08X", &breakAddress);
						for (i=0; at[i]; i++)
							if (!isxdigit(at[i])){
								sprintf(at, "%08X", breakAddress);
								Edit_SetText((HWND)lParam, at);
								break;
							}
					}else{char cycle[10+1];
						Edit_GetText((HWND)lParam, cycle, 10+1);
						sscanf (cycle, "%d", &breakCycle);
					}
			}else
				switch(wParam){
					case IDC_DEBUGEE:
						debuggedCpu=0;
						MessageBox(hDlg, _("In debugger:\nPut EE in stop state\nand IOP in run state"), _("Notice on debugging EE"), 0);
						break;
					case IDC_DEBUGIOP:
						debuggedCpu=1;	
						MessageBox(hDlg, _("In debugger:\nPut IOP in stop state\nand EE in run state"), _("Notice on debugging IOP"), 0);
						break;
					case IDC_CLEAR:
						ListBox_ResetContent(GetDlgItem(hDlg, IDC_COMMUNICATION));
						break;
					case IDOK:
						//close socket and stuff
						connected=0;
						CloseHandle(th);
						CloseHandle(runth);
						CloseHandle(runEvent);
						closesocket(serversocket);
						WSACleanup();
						ClosePlugins();
						EndDialog(hDlg, TRUE );
						return TRUE;
				}
			break;
		}
	return FALSE;
}

LRESULT WINAPI RemoteDebuggerParamsProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam){
	char strport[6+1];//short value

	switch(uMsg){
		case WM_INITDIALOG:{
			ShowCursor(TRUE);
			sprintf(strport, "%d", port);
			Edit_LimitText(GetDlgItem(hDlg, IDC_PORT), 5);
			Edit_SetText(GetDlgItem(hDlg, IDC_PORT), strport);
			Button_Enable(GetDlgItem(hDlg, IDC_DEBUGBIOS), TRUE);
			return TRUE;
		}

		case WM_COMMAND:
			switch(wParam){
				case IDOK:
					Edit_GetText(GetDlgItem(hDlg, IDC_PORT), strport, 6);
					strport[5]=0;
					port=atol(strport);
					port=min(port,65535);

					EndDialog(hDlg, (Button_GetCheck(GetDlgItem(hDlg, IDC_DEBUGBIOS))!=0)+1);
					return TRUE;
				case IDCANCEL:
					EndDialog(hDlg, FALSE);
					return TRUE;
			}
			break;
	}
	return FALSE;
}
