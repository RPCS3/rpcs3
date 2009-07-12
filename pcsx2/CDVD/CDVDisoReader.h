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

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

#define MSF2SECT(m,s,f)	(((m)*60+(s)-2)*75+(f))

extern bool loadFromISO;

extern char isoFileName[256];

extern int BlockDump;
extern FILE* fdump;
extern FILE* isoFile;
extern int isoType;

extern u8 cdbuffer[];
extern u8 *pbuffer;

s32 CALLBACK ISOinit();
void CALLBACK ISOshutdown();
s32 CALLBACK ISOopen(const char* pTitle);
void CALLBACK ISOclose();
s32 CALLBACK ISOreadSubQ(u32 lsn, cdvdSubQ* subq);
s32 CALLBACK ISOgetTN(cdvdTN *Buffer);
s32 CALLBACK ISOgetTD(u8 tn, cdvdTD *Buffer);
s32 CALLBACK ISOgetDiskType();
s32 CALLBACK ISOgetTrayStatus();
s32 CALLBACK ISOctrlTrayOpen();
s32 CALLBACK ISOctrlTrayClose();
s32 CALLBACK ISOreadSector(u8* tempbuffer, u32 lsn);
s32 CALLBACK ISOgetTOC(void* toc);
s32 CALLBACK ISOreadTrack(u32 lsn, int mode);
u8* CALLBACK ISOgetBuffer();

#endif