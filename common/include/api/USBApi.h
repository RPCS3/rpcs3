/*  Pcsx2 - Pc Ps2 Emulator
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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef __USBAPI_H__
#define __USBAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

typedef void (*USBcallback)(int cycles);
typedef int  (*USBhandler)(void);

// Basic functions.
EXPORT_C_(s32)  CALLBACK USBinit(char *configpath);
EXPORT_C_(s32)  CALLBACK USBopen(void *pDisplay);
EXPORT_C_(void) CALLBACK USBclose();
EXPORT_C_(void) CALLBACK USBshutdown();
EXPORT_C_(u8)   CALLBACK USBread8(u32 addr);
EXPORT_C_(u16)  CALLBACK USBread16(u32 addr);
EXPORT_C_(u32)  CALLBACK USBread32(u32 addr);
EXPORT_C_(void) CALLBACK USBwrite8(u32 addr,  u8 value);
EXPORT_C_(void) CALLBACK USBwrite16(u32 addr, u16 value);
EXPORT_C_(void) CALLBACK USBwrite32(u32 addr, u32 value);
EXPORT_C_(void) CALLBACK USBasync(u32 cycles);

// cycles = IOP cycles before calling callback,
// if callback returns 1 the irq is triggered, else not
EXPORT_C_(void) CALLBACK USBirqCallback(USBcallback callback);
EXPORT_C_(USBhandler) CALLBACK USBirqHandler(void);
EXPORT_C_(void) CALLBACK USBsetRAM(void *mem);

// Extended functions

EXPORT_C_(s32)  CALLBACK USBfreeze(int mode, freezeData *data);
EXPORT_C_(void) CALLBACK USBconfigure();
EXPORT_C_(void) CALLBACK USBabout();
EXPORT_C_(s32)  CALLBACK USBtest();

#endif // __USBAPI_H__