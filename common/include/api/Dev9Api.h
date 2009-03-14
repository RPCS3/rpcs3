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


#ifndef __DEV9API_H__
#define __DEV9API_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
 *	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

typedef void (*DEV9callback)(int cycles);
typedef int  (*DEV9handler)(void);

// Basic functions.
// NOTE: The read/write functions CANNOT use XMM/MMX regs
// If you want to use them, need to save and restore current ones
EXPORT_C_(s32) DEV9init(char *configpath);
EXPORT_C_(s32) DEV9open(void *pDisplay);
EXPORT_C_(void) DEV9close();
EXPORT_C_(void) DEV9shutdown();
EXPORT_C_(u8) DEV9read8(u32 addr);
EXPORT_C_(u16) DEV9read16(u32 addr);
EXPORT_C_(u32) DEV9read32(u32 addr);
EXPORT_C_(void) DEV9write8(u32 addr,  u8 value);
EXPORT_C_(void) DEV9write16(u32 addr, u16 value);
EXPORT_C_(void) DEV9write32(u32 addr, u32 value);
EXPORT_C_(void) DEV9readDMA8Mem(u32 *pMem, int size);
EXPORT_C_(void) DEV9writeDMA8Mem(u32 *pMem, int size);

// cycles = IOP cycles before calling callback,
// if callback returns 1 the irq is triggered, else not
EXPORT_C_(void) DEV9irqCallback(DEV9callback callback);
EXPORT_C_(DEV9handler) DEV9irqHandler(void);

// Extended functions

EXPORT_C_(s32) DEV9freeze(int mode, freezeData *data);
EXPORT_C_(void) DEV9configure();
EXPORT_C_(void) DEV9about();
EXPORT_C_(s32)  DEV9test();

#endif // __DEV9API_H__