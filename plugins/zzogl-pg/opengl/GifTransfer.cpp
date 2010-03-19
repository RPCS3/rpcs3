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
 
#include "GifTransfer.h"
 
void GIFtag(pathInfo *path, u32 *data) {
	FUNCLOG
	
	path->tag.nloop	= data[0] & 0x7fff;
	path->tag.eop	= (data[0] >> 15) & 0x1;
	u32 tagpre		= (data[1] >> 14) & 0x1;
	u32 tagprim		= (data[1] >> 15) & 0x7ff;
	u32 tagflg		= (data[1] >> 26) & 0x3;
	path->tag.nreg	= (data[1] >> 28)<<2;
	
	if (path->tag.nreg == 0) path->tag.nreg = 64;

	gs.q = 1;

//	GS_LOG("GIFtag: %8.8lx_%8.8lx_%8.8lx_%8.8lx: EOP=%d, NLOOP=%x, FLG=%x, NREG=%d, PRE=%d\n",
//			data[3], data[2], data[1], data[0],
//			path->tag.eop, path->tag.nloop, tagflg, path->tag.nreg, tagpre);

	path->mode = tagflg+1;

	switch (tagflg) {
		case 0x0:
			path->regs = *(u64 *)(data+2);
			path->regn = 0;
			if (tagpre)
				GIFRegHandlerPRIM((u32*)&tagprim);

			break;

		case 0x1:
			path->regs = *(u64 *)(data+2);
			path->regn = 0;
			break;
	}
}

void _GSgifPacket(pathInfo *path, u32 *pMem) { // 128bit
	FUNCLOG

	int reg = (int)((path->regs >> path->regn) & 0xf);
	g_GIFPackedRegHandlers[reg](pMem);

	path->regn += 4;
	if (path->tag.nreg == path->regn) {
		path->regn = 0;
		path->tag.nloop--;
	}
}

void _GSgifRegList(pathInfo *path, u32 *pMem) { // 64bit
	FUNCLOG

	int reg;

	reg = (int)((path->regs >> path->regn) & 0xf);

	g_GIFRegHandlers[reg](pMem);
	path->regn += 4;
	if (path->tag.nreg == path->regn) {
		path->regn = 0;
		path->tag.nloop--;
	}
}

static int nPath3Hack = 0;

void CALLBACK GSgetLastTag(u64* ptag)
{
	FUNCLOG

	*(u32*)ptag = nPath3Hack;
	nPath3Hack = 0;
}

void _GSgifTransfer(pathInfo *path, u32 *pMem, u32 size)
{
	FUNCLOG

#ifdef _WIN32
	assert( g_hCurrentThread == GetCurrentThread() );
#endif

#ifdef _DEBUG
	if( conf.log & 0x20 ) {
		static int nSaveIndex=0;
		GS_LOG("%d: p:%d %x\n", nSaveIndex++, (path==&gs.path3)?3:(path==&gs.path2?2:1), size);
		int vals[4] = {0};
		for(int i = 0; i < size; i++) {
			for(int j = 0; j < 4; ++j )
				vals[j] ^= pMem[4*i+j];
		}
		GS_LOG("%x %x %x %x\n", vals[0], vals[1], vals[2], vals[3]);
	}
#endif

	while(size > 0)
	{
		//LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
		if (path->tag.nloop == 0) 
		{
			GIFtag(path, pMem);
			pMem+= 4;
			size--;

			if ((g_GameSettings & GAME_PATH3HACK) && path == &gs.path3 && gs.path3.tag.eop)
				nPath3Hack = 1;

			if (path == &gs.path1) 
			{
				if (path->mode == 1) 
				{
					// check if 0xb is in any reg, if yes, exit (kh2)
					for(int i = 0; i < path->tag.nreg; i += 4)
					{
						if (((path->regs >> i)&0xf) == 11) 
						{
							ERROR_LOG_SPAM("Invalid unpack type\n");
							path->tag.nloop = 0;
							return;
						}
					}
				}
			}

			if(path->tag.nloop == 0 ) 
			{
				if( path == &gs.path1 ) 
				{
					// ffx hack
					if( path->tag.eop )
						return;
					continue;					
				}

				if( !path->tag.eop ) 
				{
					//DEBUG_LOG("continuing from eop\n");
					continue;
				}

				// Issue 174 fix!
				continue;
			}
		}

		switch(path->mode) {
		case 1: // PACKED
		{
			assert( path->tag.nloop > 0 );
			for(; size > 0; size--, pMem += 4)
			{
				int reg = (int)((path->regs >> path->regn) & 0xf);
				
				g_GIFPackedRegHandlers[reg](pMem);

				path->regn += 4;
				if (path->tag.nreg == path->regn) 
				{
					path->regn = 0;
					if( path->tag.nloop-- <= 1 ) 
					{
						size--;
						pMem += 4;
						break;
					}
				}
			}
			break;
		}
		case 2: // REGLIST
		{
			//GS_LOG("%8.8x%8.8x %d L\n", ((u32*)&gs.regs)[1], *(u32*)&gs.regs, path->tag.nreg/4);
			assert( path->tag.nloop > 0 );
			size *= 2;
			for(; size > 0; pMem+= 2, size--)
			{
				int reg = (int)((path->regs >> path->regn) & 0xf);
				g_GIFRegHandlers[reg](pMem);
				path->regn += 4;
				if (path->tag.nreg == path->regn) 
				{
					path->regn = 0;
					if( path->tag.nloop-- <= 1 ) 
					{
						size--;
						pMem += 2;
						break;
					}
				}
			}

			if( size & 1 ) pMem += 2;
			size /= 2;
			break;
		}
		case 3: // GIF_IMAGE (FROM_VFRAM)
		case 4: // Used in the DirectX version, so we'll use it here too.
		{
			if(gs.imageTransfer >= 0 && gs.imageTransfer <= 1)
			{
				int process = min((int)size, path->tag.nloop);

				if( process > 0 ) 
				{
					if ( gs.imageTransfer ) 
						ZeroGS::TransferLocalHost(pMem, process);
					else 
						ZeroGS::TransferHostLocal(pMem, process*4);

					path->tag.nloop -= process;
					pMem += process*4; size -= process;

					assert( size == 0 || path->tag.nloop == 0 );
				}
				break;
			}
			else 
			{
				// simulate
				int process = min((int)size, path->tag.nloop);
				path->tag.nloop -= process;
				pMem += process*4; size -= process;
			}

			break;
		}
		default: // GIF_IMAGE
			GS_LOG("*** WARNING **** Unexpected GIFTag flag\n");
			assert(0);
			path->tag.nloop = 0;
			break;
		}

		if( path == &gs.path1 && path->tag.eop )
			return;
	}

	// This is case when not all data was readed from one try: VU1 to much data.
	// So we should redone reading from start
	if (path == &gs.path1 && size == 0 && path->tag.nloop > 0) {
		ERROR_LOG_SPAMA("VU1 too much data, ignore if gfx are fine %d\n", path->tag.nloop)
//	TODO: this code is not working correctly. Anyway, ringing work only in single-threadred mode.		
//		_GSgifTransfer(&gs.path1, (u32*)((u8*)pMem-0x4000), (0x4000)/16);
	}
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	FUNCLOG

	//GS_LOG("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d)\n", size, gs.path2.mode, gs.path2.tag.nloop);
	
	_GSgifTransfer(&gs.path2, pMem, size);
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	FUNCLOG

	//GS_LOG("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d)\n", size, gs.path3.mode, gs.path3.tag.nloop);

	nPath3Hack = 0;
	_GSgifTransfer(&gs.path3, pMem, size);
}

#ifdef _DEBUG
static int count = 0;
#endif

void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr)
{
	FUNCLOG

	//pathInfo *path = &gs.path1;
	
	//GS_LOG("GSgifTransfer1 0x%x (mode %d)\n", addr, path->mode);
	
//	addr &= 0x3fff;

#ifdef _DEBUG
	PRIM_LOG("count: %d\n", count);
	count++;
#endif

	gs.path1.tag.nloop = 0;
	gs.path1.tag.eop = 0;
	_GSgifTransfer(&gs.path1, (u32*)((u8*)pMem+addr), (0x4000 - addr)/16);

	if( !gs.path1.tag.eop && gs.path1.tag.nloop > 0 ) {
		assert( (addr&0xf) == 0 ); //BUG
		gs.path1.tag.nloop = 0;
		ERROR_LOG("Transfer1 - 2\n");
		return;
	}
}

