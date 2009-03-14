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


#ifndef __SPU2API_H__
#define __SPU2API_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

EXPORT_C_(s32)  SPU2init(char *configpath);
EXPORT_C_(s32)  SPU2open(void *pDisplay);
EXPORT_C_(void) SPU2close();
EXPORT_C_(void) SPU2shutdown();
EXPORT_C_(void) SPU2write(u32 mem, u16 value);
EXPORT_C_(u16)  SPU2read(u32 mem);
EXPORT_C_(void) SPU2readDMAMem(u16 *pMem, u32 size, u8 core);
EXPORT_C_(void) SPU2writeDMAMem(u16 *pMem, u32 size, u8 core);
EXPORT_C_(void) SPU2interruptDMA(u8 core);

// all addresses passed by dma will be pointers to the array starting at baseaddr
// This function is necessary to successfully save and reload the spu2 state
EXPORT_C_(void) SPU2setDMABaseAddr(uptr baseaddr);

EXPORT_C_(u32) SPU2ReadMemAddr(u8 core);
EXPORT_C_(void) SPU2WriteMemAddr(u8 core,u32 value);
EXPORT_C_(void) SPU2irqCallback(void (*SPU2callback)(),void (*DMA4callback)(),void (*DMA7callback)());

// extended funcs
// if start is true, starts recording spu2 data, else stops
// returns true if successful
EXPORT_C_(bool) SPU2setupRecording(bool start);

EXPORT_C_(void) SPU2setClockPtr(u32* ptr);

EXPORT_C_(void) SPU2async(u32 cycles);
EXPORT_C_(s32) SPU2freeze(u8 mode, freezeData *data);
EXPORT_C_(void) SPU2configure();
EXPORT_C_(void) SPU2about();
EXPORT_C_(s32)  SPU2test();

#endif