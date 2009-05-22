/*  GSnull
 *  Copyright (C) 2004-2009 PCSX2 Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>

#include <stdio.h>
#include <assert.h>

using namespace std;

#include "GS.h"
#include "null/GSnull.h"

#ifdef __LINUX__
Display *display;
int screen;
#endif
#ifdef _WIN32
HINSTANCE HInst;
HWND GShwnd;
#endif

const unsigned char version  = PS2E_GS_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 1;    // increase that with each version

static char *libraryName = "GSnull Driver";
FILE *gsLog;
Config conf;
u32 GSKeyEvent = 0;
bool GSShift = false, GSAlt = false;

string s_strIniPath="inis/GSnull.ini";
void (*GSirq)();

EXPORT_C_(u32) PS2EgetLibType() 
{
	return PS2E_LT_GS;
}

EXPORT_C_(char*) PS2EgetLibName() 
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type) 
{
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) 
{
	va_list list;

	if (!conf.Log || gsLog == NULL) return;

	va_start(list, fmt);
	vfprintf(gsLog, fmt, list);
	va_end(list);
}

EXPORT_C_(void) GSprintf(int timeout, char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	GS_LOG("GSprintf:%s", msg);
	printf("GSprintf:%s", msg);
}

void SysPrintf(const char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	GS_LOG(msg);
	printf("GSnull:%s", msg);
}

// basic funcs

EXPORT_C_(s32) GSinit()
{
	LoadConfig();
	
#ifdef GS_LOG
	gsLog = fopen("logs/gsLog.txt", "w");
	if (gsLog) setvbuf(gsLog, NULL,  _IONBF, 0);
	GS_LOG("GSnull plugin version %d,%d\n",revision,build);
	GS_LOG("GS init\n");
#endif

	SysPrintf("Initializing GSnull\n");
	return 0;
}

EXPORT_C_(void) GSshutdown()
{
#ifdef GS_LOG
	if (gsLog) fclose(gsLog);
#endif
	
	SysPrintf("Shutting down GSnull\n");
}

#ifndef __LINUX__
LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

#endif

EXPORT_C_(s32) GSopen(void *pDsp, char *Title, int multithread)
{
#ifdef GS_LOG
	GS_LOG("GS open\n");
#endif
	//assert( GSirq != NULL );
	
#ifdef __LINUX__
	display = XOpenDisplay(0);
	screen = DefaultScreen(display);

	if( pDsp != NULL ) *(Display**)pDsp = display;
#else
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
					GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
					"PS2EMU_GSNULL", NULL };
	RegisterClassEx( &wc );

	GShwnd = CreateWindowEx( WS_EX_CLIENTEDGE, "PS2EMU_GSNULL", "The title of my window",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 240, 120, NULL, NULL, wc.hInstance, NULL);

	if(GShwnd == NULL) 
	{
		GS_LOG("Failed to create window. Exiting...");
		return -1;
	}

	if( pDsp != NULL ) *(int*)pDsp = (int)GShwnd;
#endif
	SysPrintf("Opening GSnull\n");
	return 0;
}

EXPORT_C_(void) GSclose()
{
	SysPrintf("Closing GSnull\n");
#ifdef __LINUX__
	XCloseDisplay(display);
#endif
}

EXPORT_C_(void) GSirqCallback(void (*callback)()) 
{
        GSirq = callback;
}

EXPORT_C_(s32) GSfreeze(int mode, freezeData *data) 
{
	return 0;
}

EXPORT_C_(s32) GStest() 
{
	SysPrintf("Testing GSnull\n");
	return 0;
}

void ProcessMessages()
{	
#ifdef __LINUX__
	if ( GSKeyEvent ) 
		{
		int myKeyEvent = GSKeyEvent;
		bool myShift = GSShift;
		GSKeyEvent = 0;
			
		switch ( myKeyEvent ) 
		{
			case XK_F5:
			 	OnKeyboardF5(myShift);
				break;
			case XK_F6:
				OnKeyboardF6(myShift);
				break;
			case XK_F7:
				OnKeyboardF7(myShift);
				break;	
			case XK_F9:
				OnKeyboardF9(myShift);
				break;		
		}
	}
#endif
}

EXPORT_C_(void) GSvsync(int field)
{
	ProcessMessages();
}

EXPORT_C_(void) GSgifTransfer1(u32 *pMem, u32 addr)
{
	_GSgifTransfer1(pMem, addr);
}

EXPORT_C_(void) GSgifTransfer2(u32 *pMem, u32 size)
{
	_GSgifTransfer2(pMem, size);
}

EXPORT_C_(void) GSgifTransfer3(u32 *pMem, u32 size)
{
	_GSgifTransfer3(pMem, size);
}

 // returns the last tag processed (64 bits)
EXPORT_C_(void) GSgetLastTag(u64* ptag)
{
}

EXPORT_C_(void) GSgifSoftReset(u32 mask)
{
	SysPrintf("Doing a soft reset of the GS plugin.\n");
}

EXPORT_C_(void) GSreadFIFO(u64 *mem)
{
}

EXPORT_C_(void) GSreadFIFO2(u64 *mem, int qwc)
{
}

// extended funcs

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
EXPORT_C_(void) GSkeyEvent(keyEvent *ev)
{
#ifdef __LINUX__
	switch(ev->evt) {
		case KEYPRESS:
			switch(ev->key) {
				case XK_F5:
				case XK_F6:
				case XK_F7:
				case XK_F9:
					GSKeyEvent = ev->key ;
					break;
				case XK_Escape:
					break;
				case XK_Shift_L:
				case XK_Shift_R:
					//bShift = true;
					GSShift = true;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					GSAlt = true;
					break;
				}
			break;
		case KEYRELEASE:
			switch(ev->key) {
				case XK_Shift_L:
				case XK_Shift_R:
					//bShift = false;
					GSShift = false;
					break;
				case XK_Alt_L:
				case XK_Alt_R:
					GSAlt = false;
					break;
			}
	}
#endif
}

EXPORT_C_(void) GSchangeSaveState(int, const char* filename)
{
}

EXPORT_C_(void) GSmakeSnapshot(char *path)
{
	
	SysPrintf("Taking a snapshot.\n");
}

EXPORT_C_(void) GSmakeSnapshot2(char *pathname, int* snapdone, int savejpg)
{
	SysPrintf("Taking a snapshot to %s.\n", pathname);
}

EXPORT_C_(void) GSsetBaseMem(void*)
{
}

EXPORT_C_(void) GSsetGameCRC(int crc, int gameoptions)
{
	SysPrintf("Setting the crc to '%x' with 0x%x for options.\n", crc, gameoptions);
}

// controls frame skipping in the GS, if this routine isn't present, frame skipping won't be done
EXPORT_C_(void) GSsetFrameSkip(int frameskip)
{
	SysPrintf("Frameskip set to %d.\n", frameskip);
}

// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
EXPORT_C_(int) GSsetupRecording(int start, void* pData)
{
	if (start)
		SysPrintf("Pretending to record.\n");
	else 
		SysPrintf("Pretending to stop recording.\n");
	
	return 1;
}

EXPORT_C_(void) GSreset()
{
	SysPrintf("Doing a reset of the GS plugin.");
}

EXPORT_C_(void) GSwriteCSR(u32 value)
{
}

EXPORT_C_(void) GSgetDriverInfo(GSdriverInfo *info)
{
}

#ifdef _WIN32
EXPORT_C_(s32) GSsetWindowInfo(winInfo *info)
{
	return 0;
}
#endif