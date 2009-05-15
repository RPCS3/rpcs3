/*  USBlinuz
 *  Copyright (C) 2002-2005  USBlinuz Team
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

#include "USB.h"

const unsigned char version  = PS2E_USB_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 6;    // increase that with each version

static char *libraryName     = "USBnull Driver";
string s_strIniPath="inis/USBnull.ini";
//void (*USBirq)();
USBcallback USBirq;
Config conf;
FILE *usbLog;

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_USB;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

void __Log(char *fmt, ...)
{
	va_list list;

	if (!conf.Log || usbLog == NULL) return;

	va_start(list, fmt);
	vfprintf(usbLog, fmt, list);
	va_end(list);
}

EXPORT_C_(s32) USBinit()
{
	LoadConfig();

#ifdef USB_LOG
	usbLog = fopen("logs/usbLog.txt", "w");
	if (usbLog) setvbuf(usbLog, NULL,  _IONBF, 0);
	USB_LOG("usbnull plugin version %d,%d\n", revision, build);
	USB_LOG("USBinit\n");
#endif

	return 0;
}

EXPORT_C_(void) USBshutdown()
{
#ifdef USB_LOG
	if (usbLog) fclose(usbLog);
#endif
}

EXPORT_C_(s32) USBopen(void *pDsp)
{
	USB_LOG("USBopen\n");

	return 0;
}

EXPORT_C_(void) USBclose()
{
}

EXPORT_C_(u8 ) USBread8(u32 addr)
{
	USB_LOG("*Unknown 8bit read at address %lx ", addr);
	return 0;
}

EXPORT_C_(u16) USBread16(u32 addr)
{
	USB_LOG("*Unknown 16bit read at address %lx", addr);
	return 0;
}

EXPORT_C_(u32) USBread32(u32 addr)
{
	USB_LOG("*Unknown 32bit read at address %lx", addr);
	return 0;
}

EXPORT_C_(void) USBwrite8(u32 addr,  u8 value)
{
	USB_LOG("*Unknown 8bit write at address %lx value %x\n", addr, value);
}

EXPORT_C_(void) USBwrite16(u32 addr, u16 value)
{
	USB_LOG("*Unknown 16bit write at address %lx value %x\n", addr, value);
}

EXPORT_C_(void) USBwrite32(u32 addr, u32 value)
{
	USB_LOG("*Unknown 32bit write at address %lx value %lx\n", addr, value);
}

EXPORT_C_(void) USBirqCallback(USBcallback callback) 
{
        USBirq = callback;
}

EXPORT_C_(int) _USBirqHandler(void)
{
	return 0;
}

EXPORT_C_(USBhandler) USBirqHandler(void)
{
	return (USBhandler)_USBirqHandler;
}

EXPORT_C_(void) USBsetRAM(void *mem)
{
	USB_LOG("*Setting ram.\n");
}

// extended funcs

EXPORT_C_(s32) USBfreeze(int mode, freezeData *data)
{
	return 0;
}


EXPORT_C_(s32) USBtest()
{
	return 0;
}

