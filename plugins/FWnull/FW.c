/*  FWnull
 *  Copyright (C) 2004-2005 PCSX2 Team
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

#include "FW.h"

const unsigned char version  = PS2E_FW_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 4;    // increase that with each version

static char *libraryName     = "FWnull Driver";

s8 *fwregs;

#define fwRs32(mem)	(*(s32*)&fwregs[(mem) & 0xffff])
#define fwRu32(mem)	(*(u32*)&fwregs[(mem) & 0xffff])


u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_FW;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.Log || fwLog == NULL) return;

	va_start(list, fmt);
	vfprintf(fwLog, fmt, list);
	va_end(list);
}

s32 CALLBACK FWinit() {
    LoadConfig();
#ifdef FW_LOG
	fwLog = fopen("logs/fwLog.txt", "w");
	if (fwLog) setvbuf(fwLog, NULL,  _IONBF, 0);
	FW_LOG("FWnull plugin version %d,%d\n",revision,build);
	FW_LOG("FW init\n");
#endif

	fwregs = (s8*)malloc(0x10000);
	if (fwregs == NULL) {
		SysMessage("Error allocating Memory\n"); return -1;
	}

	return 0;
}

void CALLBACK FWshutdown() {
	free(fwregs);

#ifdef FW_LOG
	if (fwLog) fclose(fwLog);
#endif
}

s32 CALLBACK FWopen(void *pDsp) {
#ifdef FW_LOG
	FW_LOG("FW open\n");
#endif

#ifdef _WIN32
#else
    Display* dsp = *(Display**)pDsp;
#endif

	return 0;
}

void CALLBACK FWclose() {
}




u32  CALLBACK FWread32(u32 addr) {
	u32 ret=0;

	switch (addr) {
		case 0x1f808410:
			ret = 0x8;
			break;

		default:
			ret = fwRu32(addr);
	}
#ifdef FW_LOG
	FW_LOG("FW read mem 0x%x: 0x%x\n", addr, ret);
#endif	

	return ret;
}



void CALLBACK FWwrite32(u32 addr, u32 value) {
	switch (addr) {
		default:
			fwRu32(addr) = value;
			break;
	}
#ifdef FW_LOG
	FW_LOG("FW write mem 0x%x: 0x%x\n", addr, value);
#endif
}

void CALLBACK FWirqCallback(void (*callback)()) {
	FWirq = callback;
}


s32 CALLBACK FWfreeze(int mode, freezeData *data) {
	return 0;
}


s32  CALLBACK FWtest() {
	return 0;
}

