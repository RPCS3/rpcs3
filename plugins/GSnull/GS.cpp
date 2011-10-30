/*  GSnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <string>

#include <stdio.h>
#include <assert.h>

using namespace std;

#include "GS.h"
#include "GifTransfer.h"
#include "null/GSnull.h"

const unsigned char version  = PS2E_GS_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 1;    // increase that with each version

static char *libraryName = "GSnull Driver";
Config conf;
u32 GSKeyEvent = 0;
bool GSShift = false, GSAlt = false;

string s_strIniPath="inis";
extern std::string s_strLogPath;
const char* s_iniFilename = "GSnull.ini";
GSVars gs;

// Because I haven't bothered to get GSOpen2 working in Windows yet in GSNull.
#ifdef __LINUX__
#define USE_GSOPEN2
#endif

void (*GSirq)();
extern void ResetRegs();
extern void SetMultithreaded();
extern void SetFrameSkip(bool skip);
extern void InitPath();

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

EXPORT_C_(void) GSprintf(int timeout, char *fmt, ...)
{
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	GSLog::Print("GSprintf:%s", msg);
}

// basic funcs
EXPORT_C_(void) GSsetSettingsDir(const char* dir)
{
	s_strIniPath = (dir == NULL) ? "inis" : dir;
}

EXPORT_C_(void) GSsetLogDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs" : dir;

	// Reload the log file after updated the path
	GSLog::Close();
	GSLog::Open();
}

EXPORT_C_(s32) GSinit()
{
	LoadConfig();

	GSLog::Open();

	GSLog::WriteLn("Initializing GSnull.");
	return 0;
}

EXPORT_C_(void) GSshutdown()
{
	GSLog::WriteLn("Shutting down GSnull.");
	GSCloseWindow();
	GSLog::Close();
}

EXPORT_C_(s32) GSopen(void *pDsp, char *Title, int multithread)
{
	int err = 0;
	GSLog::WriteLn("GS open.");
	//assert( GSirq != NULL );

	err = GSOpenWindow(pDsp, Title);
	gs.MultiThreaded = multithread;

	ResetRegs();
	SetMultithreaded();
	InitPath();
	GSLog::WriteLn("Opening GSnull.");
	return err;
}

#ifdef USE_GSOPEN2
EXPORT_C_(s32) GSopen2( void *pDsp, u32 flags )
{
	GSLog::WriteLn("GS open2.");

    GSOpenWindow2(pDsp, flags);

	gs.MultiThreaded = true;

	ResetRegs();
	SetMultithreaded();
	InitPath();
	GSLog::WriteLn("Opening GSnull (2).");
	return 0;
}
#endif

EXPORT_C_(void) GSclose()
{
	GSLog::WriteLn("Closing GSnull.");

	// Better to only close the window on Shutdown.  All the other plugins
	// pretty much worked that way, and all old PCSX2 versions expect it as well.
	//GSCloseWindow();
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
	GSLog::WriteLn("Testing GSnull.");
	return 0;
}

EXPORT_C_(void) GSvsync(int field)
{
	GSProcessMessages();
}

 // returns the last tag processed (64 bits)
EXPORT_C_(void) GSgetLastTag(u64* ptag)
{
	*(u32*)ptag = gs.nPath3Hack;
	gs.nPath3Hack = 0;
}

EXPORT_C_(void) GSgifSoftReset(u32 mask)
{
	GSLog::WriteLn("Doing a soft reset of the GS plugin.");
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
	HandleKeyEvent(ev);
}

EXPORT_C_(void) GSchangeSaveState(int, const char* filename)
{
}

EXPORT_C_(void) GSmakeSnapshot(char *path)
{

	GSLog::WriteLn("Taking a snapshot.");
}

EXPORT_C_(void) GSmakeSnapshot2(char *pathname, int* snapdone, int savejpg)
{
	GSLog::WriteLn("Taking a snapshot to %s.", pathname);
}

EXPORT_C_(void) GSsetBaseMem(void*)
{
}

EXPORT_C_(void) GSsetGameCRC(int crc, int gameoptions)
{
	GSLog::WriteLn("Setting the crc to '%x' with 0x%x for options.", crc, gameoptions);
}

// controls frame skipping in the GS, if this routine isn't present, frame skipping won't be done
EXPORT_C_(void) GSsetFrameSkip(int frameskip)
{
	SetFrameSkip(frameskip != 0);
	GSLog::WriteLn("Frameskip set to %d.", frameskip);
}

// if start is 1, starts recording spu2 data, else stops
// returns a non zero value if successful
// for now, pData is not used
EXPORT_C_(int) GSsetupRecording(int start, void* pData)
{
	if (start)
		GSLog::WriteLn("Pretending to record.");
	else
		GSLog::WriteLn("Pretending to stop recording.");

	return 1;
}

EXPORT_C_(void) GSreset()
{
	GSLog::WriteLn("Doing a reset of the GS plugin.");
}

EXPORT_C_(void) GSwriteCSR(u32 value)
{
}

EXPORT_C_(void) GSgetDriverInfo(GSdriverInfo *info)
{
}
