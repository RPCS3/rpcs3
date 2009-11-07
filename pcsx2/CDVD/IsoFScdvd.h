/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 * 
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Linux kernel headers
 *  Modified by Florin for PCSX2 emu
 */

#ifndef _ISOFSCDVD_H
#define _ISOFSCDVD_H

#include "Common.h"

#ifdef _MSC_VER
#	pragma pack(1)
#endif

struct TocEntry
{
	u32		fileLBA;
	u32		fileSize;
	u8		fileProperties;
	u8		padding1[3];
	char	filename[128+1];
	u8		date[7];
} __packed;

#ifdef _MSC_VER
#	pragma pack()
#endif

extern int IsoFS_findFile(const char* fname, struct TocEntry* tocEntry);
extern int IsoFS_readSectors(u32 lsn, u32 sectors, void *buf);

#endif // _ISOFSCDVD_H
