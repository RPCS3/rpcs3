/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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
/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Linux kernel headers
 *  Modified by Florin for PCSX2 emu
 */

#ifndef _ISOFSCDVD_H
#define _ISOFSCDVD_H

#include "Common.h"

#if defined(_MSC_VER)
#pragma pack(1)
#endif

struct TocEntry
{
	u32	fileLBA;
	u32 fileSize;
	u8	fileProperties;
	u8	padding1[3];
	char	filename[128+1];
	u8	date[7];
#if defined(_MSC_VER)
};
#else
} __attribute__((packed));
#endif

#if defined(_MSC_VER)
#pragma pack()
#endif

int IsoFS_findFile(const char* fname, struct TocEntry* tocEntry);
int IsoFS_readSectors(u32 lsn, u32 sectors, void *buf);

#endif // _ISOFSCDVD_H
