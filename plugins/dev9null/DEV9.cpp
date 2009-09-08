/*  DEV9null
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
using namespace std;

#include "DEV9.h"

const unsigned char version  = PS2E_DEV9_VERSION;
const unsigned char revision = 0;
const unsigned char build = 4;    // increase that with each version

const char *libraryName = "DEV9null Driver";
string s_strIniPath="inis/DEV9null.ini";

void (*DEV9irq)(int);
FILE *dev9Log;
Config conf;

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

	if (!conf.Log || dev9Log == NULL) return;

	va_start(list, fmt);
	vfprintf(dev9Log, fmt, list);
	va_end(list);
}

EXPORT_C_(s32) DEV9init() 
{
#ifdef __LINUX__		// for until we get a win32 version sorted out / implemented...
	LoadConfig();
#endif

#ifdef DEV9_LOG
	dev9Log = fopen("logs/dev9Log.txt", "w");
	if (dev9Log) setvbuf(dev9Log, NULL,  _IONBF, 0);
	 DEV9_LOG("dev9null plugin version %d,%d\n", revision, build);
	 DEV9_LOG("DEV9init\n");
#endif

	return 0;
}

EXPORT_C_(void) DEV9shutdown() 
{
#ifdef DEV9_LOG
	if (dev9Log) fclose(dev9Log);
#endif
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
	DEV9_LOG("*Unknown 8bit read at address %lx ", addr);
	return 0;
}

EXPORT_C_(u16 ) DEV9read16(u32 addr) 
{
	DEV9_LOG("*Unknown 16bit read at address %lx ", addr);
	return 0;
}

EXPORT_C_(u32 ) DEV9read32(u32 addr) 
{
	DEV9_LOG("*Unknown 32bit read at address %lx ", addr);
	return 0;
}

EXPORT_C_(void) DEV9write8(u32 addr,  u8 value) 
{
	DEV9_LOG("*Unknown 8bit write at address %lx value %x\n", addr, value);
}

EXPORT_C_(void) DEV9write16(u32 addr, u16 value) 
{
	DEV9_LOG("*Unknown 16bit write at address %lx value %x\n", addr, value);
}

EXPORT_C_(void) DEV9write32(u32 addr, u32 value) 
{
	DEV9_LOG("*Unknown 32bit write at address %lx value %x\n", addr, value);
}

EXPORT_C_(void) DEV9readDMA8Mem(u32 *pMem, int size) 
{
	DEV9_LOG("Reading DMA8 Mem.");
}

EXPORT_C_(void) DEV9writeDMA8Mem(u32* pMem, int size) 
{
	DEV9_LOG("Writing DMA8 Mem.");
}

EXPORT_C_(void) DEV9irqCallback(DEV9callback callback) 
{
	DEV9irq = callback;
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

#ifdef _WIN32
EXPORT_C_(void) DEV9configure()
{
	SysMessage("Nothing to Configure");
}

EXPORT_C_(void) DEV9about()
{
}

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
