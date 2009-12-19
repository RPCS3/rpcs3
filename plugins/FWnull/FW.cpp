/*  FWnull
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
using namespace std;

#include "FW.h"

const u8 version  = PS2E_FW_VERSION;
const u8 revision = 0;
const u8 build    = 5;    // increase that with each version

static char *libraryName = "FWnull Driver";

s8 *fwregs;
FILE *fwLog;
Config conf;

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

void __Log(char *fmt, ...) 
{
	va_list list;

	if (!conf.Log || fwLog == NULL) return;

	va_start(list, fmt);
	vfprintf(fwLog, fmt, list);
	va_end(list);
}

EXPORT_C_(s32) FWinit() 
{
	LoadConfig();
	
#ifdef FW_LOG
	fwLog = fopen("logs/fwLog.txt", "w");
	if (fwLog) setvbuf(fwLog, NULL,  _IONBF, 0);
	FW_LOG("FWnull plugin version %d,%d\n",revision,build);
	FW_LOG("FW init\n");
#endif

	fwregs = (s8*)malloc(0x10000);
	if (fwregs == NULL) 
	{
		SysMessage("Error allocating Memory\n"); 
		return -1;
	}

	return 0;
}

EXPORT_C_(void) FWshutdown() 
{
	free(fwregs);

#ifdef FW_LOG
	if (fwLog) fclose(fwLog);
#endif
}

EXPORT_C_(s32) FWopen(void *pDsp) 
{
#ifdef FW_LOG
	FW_LOG("FW open\n");
#endif

	return 0;
}

EXPORT_C_(void) FWclose() 
{
}

EXPORT_C_(u32) FWread32(u32 addr) 
{
	u32 ret = 0;

	switch (addr) 
	{
		case 0x1f808410:
			ret = 0x8;
			break;

		default:
			ret = fwRu32(addr);
			break;
	}
	
	FW_LOG("FW read mem 0x%x: 0x%x\n", addr, ret);

	return ret;
}

EXPORT_C_(void) FWwrite32(u32 addr, u32 value) 
{
	switch (addr) 
	{
		default:
			fwRu32(addr) = value;
			break;
	}
	FW_LOG("FW write mem 0x%x: 0x%x\n", addr, value);
}

EXPORT_C_(void) FWirqCallback(void (*callback)()) 
{
	FWirq = callback;
}

EXPORT_C_(s32) FWfreeze(int mode, freezeData *data) 
{
	return 0;
}

EXPORT_C_(s32) FWtest() 
{
	return 0;
}
