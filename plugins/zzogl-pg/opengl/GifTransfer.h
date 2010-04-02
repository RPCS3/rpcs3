/*	ZeroGS KOSMOS
 *	Copyright (C) 2005-2006 zerofrog@gmail.com
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
 */
 
#ifndef GIFTRANSFER_H_INCLUDED
#define GIFTRANSFER_H_INCLUDED

#include "GS.h"
#include "Mem.h"
#include "Regs.h"
#include "zerogs.h"
#include "Util.h"

enum GIF_FLG
{
	GIF_FLG_PACKED	= 0,
	GIF_FLG_REGLIST	= 1,
	GIF_FLG_IMAGE	= 2,
	GIF_FLG_IMAGE2	= 3
};

//
// GIFTag
REG128(GIFTag)
	u32 NLOOP:15;
	u32 EOP:1;
	u32 _PAD1:16;
	u32 _PAD2:14;
	u32 PRE:1;
	u32 PRIM:11;
	u32 FLG:2; // enum GIF_FLG
	u32 NREG:4;
	u64 REGS:64;
REG_END

void GIFtag(pathInfo *path, u32 *data);
void _GSgifPacket(pathInfo *path, u32 *pMem);
void _GSgifRegList(pathInfo *path, u32 *pMem);
void _GSgifTransfer(pathInfo *path, u32 *pMem, u32 size);

extern GIFRegHandler g_GIFPackedRegHandlers[];
extern GIFRegHandler g_GIFRegHandlers[];

#endif // GIFTRANSFER_H_INCLUDED
