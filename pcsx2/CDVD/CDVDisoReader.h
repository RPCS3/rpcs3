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
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef __CDVD_ISO_READER_H__
#define __CDVD_ISO_READER_H__

#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#include <stdio.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFileFormats.h"

#define CDVD_LOG __Log

#ifndef MAX_PATH
#define MAX_PATH 255
#endif

extern FILE *cdvdLog;

void __Log(char *fmt, ...);

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

extern char isoFileName[256];
extern isoFile *iso;

extern s32  ISOinit();
extern void ISOshutdown();
extern s32  ISOopen(const char* pTitle);
extern void ISOclose();
extern s32  ISOreadSubQ(u32 lsn, cdvdSubQ* subq);
extern s32  ISOgetTN(cdvdTN *Buffer);
extern s32  ISOgetTD(u8 tn, cdvdTD *Buffer);
extern s32  ISOgetDiskType();
extern s32  ISOgetTrayStatus();
extern s32  ISOctrlTrayOpen();
extern s32  ISOctrlTrayClose();
extern s32  ISOreadSector(u8* tempbuffer, u32 lsn, int mode);
extern s32  ISOgetTOC(void* toc);
extern s32  ISOreadTrack(u32 lsn, int mode);
extern s32  ISOgetBuffer(u8* buffer);

#endif