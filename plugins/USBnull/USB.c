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

#include "USB.h"



const unsigned char version  = PS2E_USB_VERSION;
const unsigned char revision = 0;
const unsigned char build    = 5;    // increase that with each version

static char *libraryName     = "USBnull Driver";

u32 CALLBACK PS2EgetLibType() {
	return PS2E_LT_USB;
}

char* CALLBACK PS2EgetLibName() {
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type) {
	return (version<<16) | (revision<<8) | build;
}

void __Log(char *fmt, ...) {
	va_list list;

	if (!conf.Log || usbLog == NULL) return;

	va_start(list, fmt);
	vfprintf(usbLog, fmt, list);
	va_end(list);
}

s32 CALLBACK USBinit() {
    LoadConfig();

#ifdef USB_LOG
	usbLog = fopen("logs/usbLog.txt", "w");
	if (usbLog) setvbuf(usbLog, NULL,  _IONBF, 0);
	USB_LOG("usbnull plugin version %d,%d\n",revision,build);
	USB_LOG("USBinit\n");
#endif

	return 0;
}

void CALLBACK USBshutdown() {
#ifdef USB_LOG
	if (usbLog) fclose(usbLog);
#endif
}

s32 CALLBACK USBopen(void *pDsp) {
#ifdef USB_LOG
	USB_LOG("USBopen\n");
#endif

#ifdef _WIN32
#else
 Display* dsp = *(Display**)pDsp;
#endif

	return 0;
}

void CALLBACK USBclose() {
}

u8   CALLBACK USBread8(u32 addr) {
	


#ifdef USB_LOG
	USB_LOG("*UnKnown 8bit read at address %lx ", addr);
#endif
	return 0;
}

u16  CALLBACK USBread16(u32 addr) {
	


#ifdef USB_LOG
	USB_LOG("*UnKnown 16bit read at address %lx", addr);
#endif
	return 0;
}

u32  CALLBACK USBread32(u32 addr) {

#ifdef USB_LOG
	USB_LOG("*UnKnown 32bit read at address %lx", addr);
#endif
	return 0;
}

void CALLBACK USBwrite8(u32 addr,  u8 value) {

#ifdef USB_LOG
	USB_LOG("*UnKnown 8bit write at address %lx value %x\n", addr, value);
#endif
}

void CALLBACK USBwrite16(u32 addr, u16 value) {

#ifdef USB_LOG
	USB_LOG("*UnKnown 16bit write at address %lx value %x\n", addr, value);
#endif
}

void CALLBACK USBwrite32(u32 addr, u32 value) {

#ifdef USB_LOG
	USB_LOG("*UnKnown 32bit write at address %lx value %lx\n", addr, value);
#endif
}

void CALLBACK USBirqCallback(USBcallback callback) {
	USBirq = callback;
}

int CALLBACK _USBirqHandler(void) {

	return 0;
}

USBhandler CALLBACK USBirqHandler(void) {
	return (USBhandler)_USBirqHandler;
}

void CALLBACK USBsetRAM(void *mem) {

}

// extended funcs



s32 CALLBACK USBfreeze(int mode, freezeData *data) {


	return 0;
}


s32  CALLBACK USBtest() {
	return 0;
}

