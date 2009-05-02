/*  DEV9null
 *  Copyright (C) 2002-2009  Pcsx2 Team
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

#include <stdio.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <string.h>
#include "PS2Etypes.h"
#include "DEV9.h"

const unsigned char version  = PS2E_DEV9_VERSION;
const unsigned char revision = 0;
const unsigned char build = 4;    // increase that with each version

const char *libraryName = "DEV9null Driver";

void (*DEV9irq)();
FILE *dev9Log;

EXPORT_C_(u32) PS2EgetLibType() 
{
	return PS2E_LT_DEV9;
}

EXPORT_C_(char*) PS2EgetLibName() 
{
	return (char *)libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type) 
{
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) 
{
	va_list list;

	va_start(list, fmt);
	vfprintf(dev9Log, fmt, list);
	va_end(list);
}

EXPORT_C_(s32) DEV9init() 
{
	return 0;
}

EXPORT_C_(void) DEV9shutdown() 
{
}

EXPORT_C_(s32) DEV9open(void *pDsp)
{
	return 0;
}

EXPORT_C_(void) DEV9close() 
{
	
}

EXPORT_C_(u8) DEV9read8(u32 addr) 
{
	return 0;
}

EXPORT_C_(u16 ) DEV9read16(u32 addr) 
{
	return 0;
}

EXPORT_C_(u32 ) DEV9read32(u32 addr) 
{
	return 0;
}

EXPORT_C_(void) DEV9write8(u32 addr,  u8 value) 
{
}

EXPORT_C_(void) DEV9write16(u32 addr, u16 value) 
{
}

EXPORT_C_(void) DEV9write32(u32 addr, u32 value) 
{
}

EXPORT_C_(void) DEV9readDMA8Mem(u32 *pMem, int size) 
{
}

EXPORT_C_(void) DEV9writeDMA8Mem(u32* pMem, int size) 
{
}

EXPORT_C_(void) DEV9irqCallback(DEV9callback callback) 
{
	
}

EXPORT_C_(DEV9handler) DEV9irqHandler(void) 
{
	return NULL;
}

// extended funcs

EXPORT_C_(s32) DEV9test() 
{
	return 0;
}

EXPORT_C_(void) DEV9configure()
{
	SysMessage("Nothing to Configure");
}

EXPORT_C_(void) DEV9about()
{
}

#ifdef _WIN32

HINSTANCE hInst;

void SysMessage(const char *fmt, ...) 
{
	va_list list;
	char tmp[512];
	va_start(list,fmt);
	vsprintf(tmp,fmt,list);
	va_end(list);
	MessageBox(0, tmp, "DEV9null Msg", 0);
}

BOOL APIENTRY DllMain(HANDLE hModule,                  // DLL INIT
                      DWORD  dwReason, 
                      LPVOID lpReserved) {
	hInst = (HINSTANCE)hModule;
	return TRUE;                                          // very quick :)
}

#endif
