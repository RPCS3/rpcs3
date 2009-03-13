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
 
 
#ifndef __GSAPI_H__
#define __GSAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */
 
#include "Pcsx2Api.h"

typedef struct _GSdriverInfo {
	char name[8];
	void *common;
} GSdriverInfo;

// Basic functions.
EXPORT_C_(s32)   GSinit(char *configpath);
EXPORT_C_(s32)   GSopen(char *Title, bool multithread);
EXPORT_C_(void)  GSclose();
EXPORT_C_(void)  GSshutdown();
EXPORT_C_(void)  GSvsync(int field);
EXPORT_C_(void)  GSgifTransfer1(u32 *pMem, u32 addr);
EXPORT_C_(void)  GSgifTransfer2(u32 *pMem, u32 size);
EXPORT_C_(void)  GSgifTransfer3(u32 *pMem, u32 size);
EXPORT_C_(void)  GSgetLastTag(u64* ptag); // returns the last tag processed (64 bits)
EXPORT_C_(void)  GSgifSoftReset(u32 mask);
EXPORT_C_(void)  GSreadFIFO(u64 *mem);
EXPORT_C_(void)  GSreadFIFO2(u64 *mem, int qwc);

// Extended functions

// GSkeyEvent gets called when there is a keyEvent from the PAD plugin
EXPORT_C_(void)  GSkeyEvent(keyEvent *ev);
EXPORT_C_(void)  GSchangeSaveState(s32 state, const char* filename);
EXPORT_C_(void)  GSmakeSnapshot(char *path);
EXPORT_C_(void)  GSmakeSnapshot2(char *pathname, int* snapdone, int savejpg);
EXPORT_C_(void)  GSirqCallback(void (*callback)());
EXPORT_C_(void) CALLBACK GSprintf(s32 timeout, char *fmt, ...);
EXPORT_C_(void)  GSsetBaseMem(void*);
EXPORT_C_(void)  GSsetGameCRC(s32 crc, s32 gameoptions);

// controls frame skipping in the GS, if this routine isn't present, frame skipping won't be done
EXPORT_C_(void)  GSsetFrameSkip(int frameskip);

// if start is true, starts recording spu2 data, else stops
// returns true if successful
// for now, pData is not used
EXPORT_C_(bool)  GSsetupRecording(bool start);

EXPORT_C_(void)  GSreset();
EXPORT_C_(void)  GSwriteCSR(u32 value);
EXPORT_C_(void ) GSgetDriverInfo(GSdriverInfo *info);
#ifdef _WIN32
EXPORT_C_(s32)  CALLBACK GSsetWindowInfo(winInfo *info);
#endif
EXPORT_C_(s32)   GSfreeze(u8 mode, freezeData *data);
EXPORT_C_(void)  GSconfigure();
EXPORT_C_(void)  GSabout();
EXPORT_C_(s32)   GStest();

#endif // __GSAPI_H__