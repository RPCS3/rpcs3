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
	ISOFLAGS_BLOCKDUMP =	0x0004,
	ISOFLAGS_MULTI =		0x0008,
	ISOFLAGS_BZ2 =			0x0010
};

#define CD_FRAMESIZE_RAW	2352
#define DATA_SIZE	(CD_FRAMESIZE_RAW-12)

//#define itob(i)		((i)/10*16 + (i)%10)	/* u_char to BCD */
//#define btoi(b)		((b)/16*10 + (b)%16)	/* BCD to u_char */

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
	u32  offset;
	u32  blockofs;
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


isoFile *isoOpen(const char *filename);
isoFile *isoCreate(const char *filename, int mode);
int  isoSetFormat(isoFile *iso, int blockofs, int blocksize, int blocks);
int  isoDetect(isoFile *iso);
int  isoReadBlock(isoFile *iso, u8 *dst, int lsn);
int  isoWriteBlock(isoFile *iso, u8 *src, int lsn);
void isoClose(isoFile *iso);

#endif /* __LIBISO_H__ */
