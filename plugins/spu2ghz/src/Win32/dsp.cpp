//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#include "spu2.h"

extern "C" {
#include "dsp.h"

typedef winampDSPHeader* (*pWinampDSPGetHeader2)();
}

HMODULE hLib = NULL;
pWinampDSPGetHeader2 pGetHeader = NULL;
winampDSPHeader* pHeader = NULL;

winampDSPModule* pModule = NULL;

HWND hTemp;

#define USE_A_THREAD
#ifdef USE_A_THREAD

HANDLE hUpdateThread;
DWORD UpdateThreadId;

bool running;

DWORD WINAPI DspUpdateThread(PVOID param);
#endif
s32 DspLoadLibrary(char *fileName, int modNum)
#ifdef USE_A_THREAD
{
	if(!dspPluginEnabled) return -1;

	running=true;
	hUpdateThread = CreateThread(NULL,0,DspUpdateThread,NULL,0,&UpdateThreadId);
	return (hUpdateThread==INVALID_HANDLE_VALUE);
}

s32 DspLoadLibrary2(char *fileName, int modNum)
#endif
{
	if(!dspPluginEnabled) return -1;

	hLib = LoadLibraryA(fileName);
	if(!hLib)
	{
		return 1;
	}

	pGetHeader = (pWinampDSPGetHeader2)GetProcAddress(hLib,"winampDSPGetHeader2");

	if(!pGetHeader)
	{
		FreeLibrary(hLib);
		hLib=NULL;
		return 1;
	}

	pHeader = pGetHeader();

	pModule = pHeader->getModule(modNum);

	if(!pModule)
	{
		pGetHeader=NULL;
		pHeader=NULL;
		FreeLibrary(hLib);
		hLib=NULL;
		return -1;
	}

	pModule->hDllInstance = hLib;
	pModule->hwndParent=0;
	pModule->Init(pModule);

	return 0;
}

void DspCloseLibrary()
#ifdef USE_A_THREAD
{
	if(!dspPluginEnabled) return ;

	PostThreadMessage(UpdateThreadId,WM_QUIT,0,0);
	running=false;
	if(WaitForSingleObject(hUpdateThread,1000)==WAIT_TIMEOUT)
	{
		TerminateThread(hUpdateThread,1);
	}
}

void DspCloseLibrary2()
#endif
{
	if(!dspPluginEnabled) return ;

	if(hLib)
	{
		pModule->Quit(pModule);
		FreeLibrary(hLib);
	}
	pModule=NULL;
	pHeader=NULL;
	pGetHeader=NULL;
	hLib=NULL;
}

int DspProcess(s16 *buffer, int samples)
{
	if(!dspPluginEnabled) return samples;

	if(hLib)
	{
		return pModule->ModifySamples(pModule,buffer,samples,16,2,SampleRate);
	}
	return samples;
}

void DspUpdate()
#ifdef USE_A_THREAD
{
}

DWORD WINAPI DspUpdateThread(PVOID param)
{
	if(!dspPluginEnabled) return -1;

	if(DspLoadLibrary2(dspPlugin,dspPluginModule))
		return -1;

	MSG msg;
	while(running)
	{
		GetMessage(&msg,0,0,0);
		if((msg.hwnd==NULL)&&(msg.message==WM_QUIT))
		{
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DspCloseLibrary2();
	return 0;
}

#else
{
	if(!dspPluginEnabled) return;

	MSG msg;
	while(PeekMessage(&msg,0,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

#endif