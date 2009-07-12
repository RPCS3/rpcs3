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

#ifdef _WIN32
#include <windows.h>
#endif

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFileFormats.h"

#define CDVD_LOG __Log
extern FILE *cdvdLog;

void __Log(char *fmt, ...);

#define VERBOSE 1

typedef struct
{
	int slsn;
	int elsn;
#ifdef _WIN32
	HANDLE handle;
#else
	FILE *handle;
#endif
} _cdIso;

extern _cdIso cdIso[8];

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f))

extern const u8 version;
extern const u8 revision;
extern const u8 build;

extern char isoFileName[256];
extern char IsoCWD[256];
extern char CdDev[256];

extern int BlockDump;
extern isoFile *blockDumpFile;
extern isoFile *iso;

extern u8 cdbuffer[];
extern u8 *pbuffer;
extern int cdblocksize;
extern int cdblockofs;
extern int cdoffset;
extern int cdtype;
extern int cdblocks;

extern int Zmode; // 1 Z - 2 bz2
extern int fmode;						// 0 - file / 1 - Zfile
extern char *Ztable;

extern char *methods[];

s32  ISOinit();
void ISOshutdown();
s32  ISOopen(const char* pTitle);
void ISOclose();
s32  ISOreadSubQ(u32 lsn, cdvdSubQ* subq);
s32  ISOgetTN(cdvdTN *Buffer);
s32  ISOgetTD(u8 tn, cdvdTD *Buffer);
s32  ISOgetDiskType();
s32  ISOgetTrayStatus();
s32  ISOctrlTrayOpen();
s32  ISOctrlTrayClose();
s32  ISOreadSector(u8* tempbuffer, u32 lsn, int mode);
s32  ISOgetTOC(void* toc);
s32  ISOreadTrack(u32 lsn, int mode);
s32  ISOgetBuffer(u8* buffer);

#endif