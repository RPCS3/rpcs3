/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#ifndef __LIBISO_H__
#define __LIBISO_H__

#include "CDVD.h"
#include "IsoFileTools.h"

enum isoType
{
	ISOTYPE_ILLEGAL = 0,
	ISOTYPE_CD,
	ISOTYPE_DVD,
	ISOTYPE_AUDIO,
	ISOTYPE_DVDDL
};

enum isoFlags
{
	ISOFLAGS_Z =			0x0001,
	ISOFLAGS_Z2	=			0x0002,
	ISOFLAGS_BLOCKDUMP_V2 =	0x0004,
	ISOFLAGS_MULTI =		0x0008,
	ISOFLAGS_BZ2 =			0x0010,
	ISOFLAGS_BLOCKDUMP_V3 =	0x0020
};

static const int CD_FRAMESIZE_RAW	= 2448;

struct _multih
{
	u32 slsn;
	u32 elsn;
	void *handle;
};

struct isoFile
{
	char filename[256];
	isoType type;
	u32  flags;
	s32  offset;
	s32  blockofs;
	u32  blocksize;
	u32  blocks;
	void *handle;
	void *htable;
	char *Ztable;
	u32  *dtable;
	int  dtablesize;
	_multih multih[8];
	int  buflsn;
	u8 *buffer;
};


extern isoFile *isoOpen(const char *filename);
extern isoFile *isoCreate(const char *filename, int mode);
extern bool isoSetFormat(isoFile *iso, int blockofs, uint blocksize, uint blocks);
extern bool isoDetect(isoFile *iso);
extern bool isoReadBlock(isoFile *iso, u8 *dst, uint lsn);
extern bool isoWriteBlock(isoFile *iso, u8 *src, uint lsn);
extern void isoClose(isoFile *iso);

#endif /* __LIBISO_H__ */
