/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
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

#include "GS.h"
#include "Mem.h"
#include "GifTransfer.h"

#ifdef _DEBUG
static int path1_count = 0;
#endif

static int nPath3Hack = 0;

void CALLBACK GSgetLastTag(u64* ptag)
{
	FUNCLOG

	*(u32*)ptag = nPath3Hack;
	nPath3Hack = 0;
}

#ifdef _WIN32
// for debug assertion checks (thread affinity checks)
extern HANDLE g_hCurrentThread;
#endif

__forceinline void gifTransferLog(int index, const u32 *pMem, u32 size)
{
#ifdef DEBUG_TRANSFER

	if (conf.log)
	{
		static int nSaveIndex = 0;
		ZZLog::Debug_Log("%d: p:%d %x", nSaveIndex++, index + 1, size);
		int vals[4] = {0};

		for (u32 i = 0; i < size; i++)
		{
			for (u32 j = 0; j < 4; ++j)
				vals[j] ^= pMem[4*i+j];
		}

		ZZLog::Debug_Log("%x %x %x %x", vals[0], vals[1], vals[2], vals[3]);
	}

#endif
}

extern int g_GSMultiThreaded;

template<int index> void _GSgifTransfer(const u32 *pMem, u32 size)
{
	FUNCLOG

	pathInfo *path = &gs.path[index];

#ifdef _WIN32
	assert(g_hCurrentThread == GetCurrentThread());
#endif

#ifdef _DEBUG
	gifTransferLog(index, pMem, size);
	const u32* start = pMem;
	u32 size_arg = size;
#endif

	while (size > 0)
	{
		//LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
		if (path->nloop == 0)
		{
			path->setTag(pMem);
			pMem += 4;
			size--;
			
			// eeuser 7.2.2. GIFtag: "... when NLOOP is 0, the GIF does not output anything, and
			// values other than the EOP field are disregarded."
			if (path->nloop > 0)
			{
				gs.q = 1.0f;

				if (path->tag.PRE && (path->tag.FLG == GIF_FLG_PACKED))
				{
					u64 tagprim = path->tag.PRIM;
					GIFRegHandlerPRIM((u32*)&tagprim);
				}
			}
		}
		else
		{
			switch (path->mode)
			{
				case GIF_FLG_PACKED:
				{
					// Needs to be looked at.

					// first try a shortcut for a very common case

					if (path->adonly && size >= path->nloop)
					{
						size -= path->nloop;

						do
						{
							GIFPackedRegHandlerA_D(pMem);

							pMem += 4; //sizeof(GIFPackedReg)/4;
						}
						while(--path->nloop > 0);
						break;
					}

					do
					{
						u32 reg = path->GetReg();
						g_GIFPackedRegHandlers[reg](pMem);
						
						pMem += 4; //sizeof(GIFPackedReg)/4;
						size--;
					}
					while (path->StepReg() && (size > 0));

					break;
				}

				case GIF_FLG_REGLIST:
				{
					// Needs to be looked at.
					//ZZLog::GS_Log("%8.8x%8.8x %d L", ((u32*)&gs.regs)[1], *(u32*)&gs.regs, path->tag.nreg/4);

					size *= 2;

					do
					{
						g_GIFRegHandlers[path->GetReg()](pMem);

						pMem += 2;
						size--;
					}
					while (path->StepReg() && (size > 0));

					if (size & 1) pMem += 2;
					size /= 2;
					break;
				}

				case GIF_FLG_IMAGE: // FROM_VFRAM
				case GIF_FLG_IMAGE2: // Used in the DirectX version, so we'll use it here too.
				{
					int len = min(size, path->nloop);
					//ZZLog::Error_Log("GIF_FLG_IMAGE(%d)=%d", gs.imageTransfer, len);

					switch (gs.imageTransfer)
					{
						case 0:
							TransferHostLocal(pMem, len * 4);
							break;

						case 1:
							// This can't happen; downloads can not be started or performed as part of
							// a GIFtag operation.  They're an entirely separate process that can only be
							// done through the ReverseFIFO transfer (aka ReadFIFO). --air
							assert(0);
							//TransferLocalHost(pMem, len);
							break;

						case 2:
							//TransferLocalLocal();
							break;

						case 3:
							//assert(0);
							break;

						default:
							//assert(0);
							break;
					}

					pMem += len * 4;

					path->nloop -= len;
					size -= len;

					break;
				}

				default: // GIF_IMAGE
					ZZLog::GS_Log("*** WARNING **** Unexpected GIFTag flag.");
					assert(0);
					path->nloop = 0;
					break;
			}
		}

		if (index == 0)
		{
			if (path->tag.EOP && path->nloop == 0)
			{
				break;
			}
		}
	}

#ifdef _DEBUG
	g_dump.Transfer(index, start, pMem - start + 4, size_arg);
#endif

	// This is the case when not all data was readed from one try: VU1 has too much data.
	// So we should redo reading from the start.
	if (index == 0)
	{
		if (size == 0 && path->nloop > 0)
		{
			if (g_GSMultiThreaded)
			{
				path->nloop = 0;
			}
			else
			{
				_GSgifTransfer<0>(pMem - 0x4000, 0x4000 / 16);
			}
		}
	}
}

void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer1 0x%x (mode %d).", addr, path->mode);

#ifdef _DEBUG
	ZZLog::Prim_Log("count: %d\n", path1_count);
	path1_count++;
#endif

	_GSgifTransfer<0>((u32*)((u8*)pMem + addr), (0x4000 - addr) / 16);
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d).", size, gs.path[1].mode, gs.path[1].tag.nloop);

	_GSgifTransfer<1>(const_cast<u32*>(pMem), size);
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d).", size, gs.path[2].mode, gs.path[2].tag.nloop);

	_GSgifTransfer<2>(const_cast<u32*>(pMem), size);
}

void CALLBACK GSgifTransfer(const u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d).", size, gs.path[2].mode, gs.path[2].tag.nloop);

	_GSgifTransfer<3>(pMem, size);
}

void InitPath()
{
	gs.path[0].mode = gs.path[1].mode = gs.path[2].mode = gs.path[3].mode = 0;
}

