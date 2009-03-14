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
 
 
#ifndef __FWAPI_H__
#define __FWAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */
 
#include "Pcsx2Api.h"

/* FW plugin API */

// Basic functions.

// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
EXPORT_C_(s32) FWinit();
// pDisplay normally is passed a handle to the GS plugins window.
EXPORT_C_(s32) FWopen(void *pDisplay);
EXPORT_C_(void) FWclose();
EXPORT_C_(void) FWshutdown();
EXPORT_C_(u32)  FWread32(u32 addr);
EXPORT_C_(void) FWwrite32(u32 addr, u32 value);
EXPORT_C_(void) FWirqCallback(void (*callback)());

// Extended functions

EXPORT_C_(void) FWkeyEvent(keyEvent *ev);
EXPORT_C_(s32)  FWfreeze(u8 mode, freezeData *data);
EXPORT_C_(void) FWconfigure();
EXPORT_C_(void) SPU2configpath(char *configpath);
EXPORT_C_(void) FWabout();
EXPORT_C_(s32)  FWtest();
#endif

#endif // __USBAPI_H__