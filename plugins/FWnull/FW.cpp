/*  FWnull
 *  Copyright (C) 2004-2010  PCSX2 Dev Team
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
#include <string>
using namespace std;

#include "FW.h"

const u8 version  = PS2E_FW_VERSION;
const u8 revision = 0;
const u8 build    = 6;    // increase that with each version

static char *libraryName = "FWnull Driver";

string s_strIniPath="inis/";

s8 *fwregs;
Config conf;
PluginLog FWLog;

void (*FWirq)();

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_FW;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version<<16) | (revision<<8) | build;
}

EXPORT_C_(s32) FWinit()
{
	LoadConfig();
	setLoggingState();
	FWLog.Open("logs/FWnull.log");
	FWLog.WriteLn("FWnull plugin version %d,%d", revision, build);
	FWLog.WriteLn("Initializing FWnull");

	// Initializing our registers.
	fwregs = (s8*)calloc(0x10000,1);
	if (fwregs == NULL)
	{
		FWLog.Message("Error allocating Memory");
		return -1;
	}
	return 0;
}

EXPORT_C_(void) FWshutdown()
{
	// Freeing the registers.
	free(fwregs);
	fwregs = NULL;

	FWLog.Close();
}

EXPORT_C_(s32) FWopen(void *pDsp)
{
	FWLog.WriteLn("Opening FWnull.");

	return 0;
}

EXPORT_C_(void) FWclose()
{
	// Close the plugin.
	FWLog.WriteLn("Closing FWnull.");
}

EXPORT_C_(u32) FWread32(u32 addr)
{
	u32 ret = 0;

	switch (addr)
	{
		// We should document what this location is.
		case 0x1f808410:
			ret = 0x8;
			break;

		// Include other relevant 32 bit addresses we need to catch here.
		default:
			// By default, read fwregs.
			ret = fwRu32(addr);
			break;
	}

	FWLog.WriteLn("FW read mem 0x%x: 0x%x", addr, ret);

	return ret;
}

EXPORT_C_(void) FWwrite32(u32 addr, u32 value)
{
	switch (addr)
	{
//		Include other memory locations we want to catch here.
//		For example:
//
//		case 0x1f808400:
//		case 0x1f808414:
//		case 0x1f808420:
//		case 0x1f808428:
//		case 0x1f808430:
//
//		Are addresses to look at.
		case 0x1f808410: fwRu32(addr) = value; break;
		default:
			// By default, just write it to fwregs.
			fwRu32(addr) = value;
			break;
	}
	FWLog.WriteLn("FW write mem 0x%x: 0x%x", addr, value);
}

EXPORT_C_(void) FWirqCallback(void (*callback)())
{
	// Register FWirq, so we can trigger an interrupt with it later.
	FWirq = callback;
}

EXPORT_C_(void) FWsetSettingsDir(const char* dir)
{
	// Find out from pcsx2 where we are supposed to put our ini file.
    s_strIniPath = (dir == NULL) ? "inis/" : dir;
}

EXPORT_C_(s32) FWfreeze(int mode, freezeData *data)
{
	// This should store or retrieve any information, for if emulation
	// gets suspended, or for savestates.
	switch(mode)
	{
		case FREEZE_LOAD:
			// Load previously saved data.
			break;
		case FREEZE_SAVE:
			// Save data.
			break;
		case FREEZE_SIZE:
			// return the size of the data.
			break;
	}
	return 0;
}

EXPORT_C_(s32) FWtest()
{
	// 0 if the plugin works, non-0 if it doesn't.
	return 0;
}
