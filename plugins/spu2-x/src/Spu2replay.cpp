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
//Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "Global.h"
#include "PS2E-spu2.h"

#ifdef _MSC_VER
#	include "Windows.h"
#endif

FILE* s2rfile;

void s2r_write16(s16 data)
{
	fwrite(&data,2,1,s2rfile);
}

void s2r_write32(u32 data)
{
	fwrite(&data,4,1,s2rfile);
}

#define EMITC(i,a) s2r_write32(((u32)(i&0x7)<<29)|(a&0x1FFFFFFF))

int s2r_open(u32 ticks, char *filename)
{
	s2rfile=fopen(filename,"wb");
	if(s2rfile)
		s2r_write32(ticks);
	return s2rfile?0:-1;
}

void s2r_readreg(u32 ticks,u32 addr)
{
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(0,addr);
}

void s2r_writereg(u32 ticks,u32 addr,s16 value)
{
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(1,addr);
	s2r_write16(value);
}

void s2r_writedma4(u32 ticks,u16*data,u32 len)
{
	u32 i;
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(2,len);
	for(i=0;i<len;i++,data++)
		s2r_write16(*data);
}

void s2r_writedma7(u32 ticks,u16*data,u32 len)
{
	u32 i;
	if(!s2rfile) return;
	s2r_write32(ticks);
	EMITC(3,len);
	for(i=0;i<len;i++,data++)
		s2r_write16(*data);
}

void s2r_close()
{
	if(!s2rfile) return;
	fclose(s2rfile);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// replay code

bool replay_mode=false;

u16 dmabuffer[0xFFFFF];

const u32 IOP_CLK = 768*48000;
const u32 IOPCiclesPerMS = 768*48;
u32 CurrentIOPCycle=0;

u64 HighResFreq;
u64 HighResPrev;
double HighResScale;

bool Running = false;

#ifdef _MSC_VER

int conprintf(const char* fmt, ...)
{
#ifdef WIN32
	char s[1024];
	va_list list;

	va_start(list, fmt);
	vsprintf(s,fmt, list);
	va_end(list);

	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if(handle == INVALID_HANDLE_VALUE)
		return 0;

	DWORD written = 0;
	WriteConsoleA(handle, s, strlen(s), &written, 0);
	FlushFileBuffers(handle);

	return written;
#else
	va_list list;
	va_start(list, fmt);
	int ret = vsprintf(stderr,fmt,list);
	va_end(list);
	return ret;
#endif
}

void dummy1()
{
}

void dummy4()
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	SPU2interruptDMA4();
#endif
}

void dummy7()
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	SPU2interruptDMA7();
#endif
}

u64 HighResFrequency()
{
	u64 freq;
#ifdef WIN32
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
#else
	// TODO
#endif
	return freq;
}

u64 HighResCounter()
{
	u64 time;
#ifdef WIN32
	QueryPerformanceCounter((LARGE_INTEGER*)&time);
#else
	// TODO
#endif
	return time;
}

void InitWaitSync() // not extremely accurate but enough.
{
	HighResFreq = HighResFrequency();
	HighResPrev = HighResCounter();
	HighResScale = (double)HighResFreq / (double)IOP_CLK;
}

u32 WaitSync( u32 TargetCycle ) 
{
	u32 WaitCycles = (TargetCycle - CurrentIOPCycle);
	u32 WaitTime = WaitCycles / IOPCiclesPerMS;
	if(WaitTime > 10)
		WaitTime = 10;
	if(WaitTime == 0)
		WaitTime = 1;
	SleepEx(WaitTime, TRUE);

	// Refresh current time after sleeping
	u64 Current = HighResCounter();
	u32 delta = (u32)floor((Current-HighResPrev) / HighResScale + 0.5); // We lose some precision here, cycles might drift away over long periods of time ;P

	// Calculate time delta
	CurrentIOPCycle += delta;
	HighResPrev += (u64)floor(delta * HighResScale + 0.5); // Trying to compensate drifting mentioned above, not necessarily useful.

	return delta;
}

#ifdef WIN32
BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
{
	Running = false;
	return TRUE;
}
#endif

#include "Windows/Dialogs.h"
EXPORT_C_(void) s2r_replay(HWND hwnd, HINSTANCE hinst, LPSTR filename, int nCmdShow)
{
#ifndef ENABLE_NEW_IOPDMA_SPU2
	int events=0;

	Running = true;

#ifdef WIN32
	AllocConsole();
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	
	conprintf("Playing %s file on %x...",filename,hwnd);

#endif

	// load file
	FILE *file=fopen(filename,"rb");

	if(!file)
	{
		conprintf("Could not open the replay file.");
		return;
	}
	// if successful, init the plugin

#define TryRead(dest,size,count,file) if(fread(dest,size,count,file)<count) { conprintf("Error reading from file.");  goto Finish;  /* Need to exit the while() loop and maybe also the switch */ }

	TryRead(&CurrentIOPCycle,4,1,file);
	
	replay_mode=true;

	InitWaitSync(); // Initialize the WaitSync stuff

	SPU2init();
	SPU2irqCallback(dummy1,dummy4,dummy7);
	SPU2setClockPtr(&CurrentIOPCycle);
	SPU2open(&hwnd);

	CurrentIOPCycle=0;

	SPU2async(0);

	while(!feof(file) && Running)
	{
		u32 ccycle=0;
		u32 evid=0;
		u32 sval=0;
		u32 tval=0;

		TryRead(&ccycle,4,1,file);
		TryRead(&sval,4,1,file);

		evid=sval>>29;
		sval&=0x1FFFFFFF;

		u32 TargetCycle = ccycle * 768;

		while(TargetCycle > CurrentIOPCycle)
		{
			u32 delta = WaitSync(TargetCycle);
			SPU2async(delta);
		}
		
		switch(evid)
		{
		case 0:
			SPU2read(sval);
			break;
		case 1:
			TryRead(&tval,2,1,file);
			SPU2write(sval,tval);
			break;
		case 2:
			TryRead(dmabuffer,sval,2,file);
			SPU2writeDMA4Mem(dmabuffer,sval);
			break;
		case 3:
			TryRead(dmabuffer,sval,2,file);
			SPU2writeDMA7Mem(dmabuffer,sval);
			break;
		default:
			// not implemented
			return;
			break;
		}
		events++;
	}

Finish:

	//shutdown
	SPU2close();
	SPU2shutdown();
	fclose(file);

	conprintf("Finished playing %s file (%d cycles, %d events).",filename,CurrentIOPCycle,events);

#ifdef WIN32
	FreeConsole();
#endif

	replay_mode=false;
#endif
}
#endif
