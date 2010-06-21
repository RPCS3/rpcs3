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

#include "GS.h"
#include "Mem.h"
#include "zerogs.h"
#include "GifTransfer.h"

#ifdef _DEBUG
static int count = 0;
#endif

/*void _GSgifPacket(pathInfo *path, u32 *pMem)  // 128bit
{
	FUNCLOG

	int reg = (int)((path->regs >> path->regn) & 0xf);
	g_GIFPackedRegHandlers[reg](pMem);

	path->regn += 4;
	if (path->nreg == path->regn)
	{
		path->regn = 0;
		path->nloop--;
	}
}

void _GSgifRegList(pathInfo *path, u32 *pMem)  // 64bit
{
	FUNCLOG

	int reg = (int)((path->regs >> path->regn) & 0xf);

	g_GIFRegHandlers[reg](pMem);

	path->regn += 4;
	if (path->nreg == path->regn)
	{
		path->regn = 0;
		path->nloop--;
	}
}*/

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

__forceinline void gifTransferLog(int index, u32 *pMem, u32 size)
{
#ifdef _DEBUG

	if (conf.log /*& 0x20*/)
	{
		static int nSaveIndex = 0;
		ZZLog::GS_Log("%d: p:%d %x", nSaveIndex++, index + 1, size);
		int vals[4] = {0};

		for (u32 i = 0; i < size; i++)
		{
			for (u32 j = 0; j < 4; ++j)
				vals[j] ^= pMem[4*i+j];
		}

		ZZLog::GS_Log("%x %x %x %x", vals[0], vals[1], vals[2], vals[3]);
	}

#endif
}

#ifdef NEW_GIF_TRANSFER
extern int g_GSMultiThreaded;

template<int index> void _GSgifTransfer(u32 *pMem, u32 size)
{
	FUNCLOG

	pathInfo *path = &gs.path[index];

#ifdef _WIN32
	assert(g_hCurrentThread == GetCurrentThread());
#endif

#ifdef _DEBUG
	gifTransferLog(index, pMem, size);
#endif

	while (size > 0)
	{
		//LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
		if (path->nloop == 0)
		{
			path->setTag(pMem);
			pMem += 4;
			size--;

			if ((conf.settings().path3) && (index == 2) && path->eop) nPath3Hack = 1;

			// eeuser 7.2.2. GIFtag: "... when NLOOP is 0, the GIF does not output anything, and
			// values other than the EOP field are disregarded."
			if (path->nloop > 0)
			{
				gs.q = 1.0f;

				if (path->tag.PRE && (path->tag.FLG == GIF_FLG_PACKED))
				{
					u32 tagprim = path->tag.PRIM;
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

					/*if(path.adonly && size >= path.nloop)
					{
						size -= path.nloop;

						do
						{
							GIFPackedRegHandlerA_D(pMem);

							mem += sizeof(GIFPackedReg);
						}
						while(--path.nloop > 0);*/

					do
					{
						g_GIFPackedRegHandlers[path->GetReg()](pMem);

						pMem += 4;
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
							ZeroGS::TransferHostLocal(pMem, len * 4);
							break;

						case 1:
							ZeroGS::TransferLocalHost(pMem, len);
							break;

						case 2:
							//Move();
							//ZZLog::Error_Log("GIF_FLG_IMAGE MOVE");
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
	ZZLog::Prim_Log("count: %d\n", count);
	count++;
#endif

	_GSgifTransfer<0>((u32*)((u8*)pMem + addr), (0x4000 - addr) / 16);
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d).", size, gs.path[1].mode, gs.path[1].tag.nloop);

	_GSgifTransfer<1>(pMem, size);
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d).", size, gs.path[2].mode, gs.path[2].tag.nloop);

	_GSgifTransfer<2>(pMem, size);
}

#else

template<int index> void _GSgifTransfer(u32 *pMem, u32 size)
{
	FUNCLOG

	pathInfo *path = &gs.path[index];

#ifdef _WIN32
	assert(g_hCurrentThread == GetCurrentThread());
#endif

#ifdef _DEBUG
	gifTransferLog(index, pMem, size);
#endif

	while (size > 0)
	{
		//LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
		if (path->nloop == 0)
		{
			path->setTag(pMem);
			gs.q = 1;
			pMem += 4;
			size--;

			if ((conf.settings() & GAME_PATH3HACK) && (index == 2) && path->eop) nPath3Hack = 1;

			if (index == 0)
			{
				if (path->mode == GIF_FLG_PACKED)
				{
					// check if 0xb is in any reg, if yes, exit (kh2)
					for (int i = 0; i < path->nreg; i += 4)
					{
						if (((path->regs >> i) & 0xf) == 11)
						{
							ERROR_LOG_SPAM("Invalid unpack type\n");
							path->nloop = 0;
							return;
						}
					}
				}
			}

			if (path->nloop == 0)
			{
				if (index == 0)
				{
					// ffx hack
					if (path->eop) return;

					continue;
				}

				/*if( !path->eop )
				{
				//ZZLog::Debug_Log("Continuing from eop.");
				continue;
				}*/

				// Issue 174 fix!
				continue;
			}
		}

		switch (path->mode)
		{

			case GIF_FLG_PACKED:
			{
				assert(path->nloop > 0);

				for (; size > 0; size--, pMem += 4)
				{
					int reg = (int)((path->regs >> path->regn) & 0xf);

					g_GIFPackedRegHandlers[reg](pMem);

					path->regn += 4;

					if (path->nreg == path->regn)
					{
						path->regn = 0;

						if (path->nloop-- <= 1)
						{
							size--;
							pMem += 4;
							break;
						}
					}
				}

				break;
			}

			case GIF_FLG_REGLIST:
			{
				//GS_LOG("%8.8x%8.8x %d L", ((u32*)&gs.regs)[1], *(u32*)&gs.regs, path->tag.nreg/4);
				assert(path->nloop > 0);
				size *= 2;

				for (; size > 0; pMem += 2, size--)
				{
					int reg = (int)((path->regs >> path->regn) & 0xf);

					g_GIFRegHandlers[reg](pMem);

					path->regn += 4;

					if (path->nreg == path->regn)
					{
						path->regn = 0;

						if (path->nloop-- <= 1)
						{
							size--;
							pMem += 2;
							break;
						}
					}
				}

				if (size & 1) pMem += 2;

				size /= 2;

				break;
			}

			case GIF_FLG_IMAGE: // FROM_VFRAM
			case GIF_FLG_IMAGE2: // Used in the DirectX version, so we'll use it here too.
			{
				if (gs.imageTransfer >= 0 && gs.imageTransfer <= 1)
				{
					int process = min((int)size, path->nloop);

					if (process > 0)
					{
						if (gs.imageTransfer)
							ZeroGS::TransferLocalHost(pMem, process);
						else
							ZeroGS::TransferHostLocal(pMem, process*4);

						path->nloop -= process;

						pMem += process * 4;

						size -= process;

						assert(size == 0 || path->nloop == 0);
					}

					break;
				}
				else
				{
					// simulate
					int process = min((int)size, path->nloop);

					path->nloop -= process;
					pMem += process * 4;
					size -= process;
				}

				break;
			}

			default: // GIF_IMAGE
				ZZLog::GS_Log("*** WARNING **** Unexpected GIFTag flag.");
				assert(0);
				path->nloop = 0;
				break;
		}

		if ((index == 0) && path->eop) return;
	}

	// This is the case when not all data was readed from one try: VU1 has too much data.
	// So we should redo reading from the start.
	if ((index == 0) && size == 0 && path->nloop > 0)
	{
		ERROR_LOG_SPAMA("VU1 too much data, ignore if gfx are fine %d\n", path->nloop)
		//      TODO: this code is not working correctly. Anyway, ringing work only in single-threaded mode.
		//              _GSgifTransfer(&gs.path[0], (u32*)((u8*)pMem-0x4000), (0x4000)/16);
	}
}

void CALLBACK GSgifTransfer1(u32 *pMem, u32 addr)
{
	FUNCLOG

	pathInfo *path = &gs.path[0];

	//ZZLog::GS_Log("GSgifTransfer1 0x%x (mode %d).", addr, path->mode);

//	addr &= 0x3fff;

#ifdef _DEBUG
	ZZLog::Prim_Log("count: %d\n", count);
	count++;
#endif

	path->nloop = 0;
	path->eop = 0;
	_GSgifTransfer<0>((u32*)((u8*)pMem + addr), (0x4000 - addr) / 16);

	if (!path->eop && (path->nloop > 0))
	{
		assert((addr&0xf) == 0);   //BUG
		path->nloop = 0;
		ZZLog::Debug_Log("Transfer1 - 2.");
		return;
	}
}

void CALLBACK GSgifTransfer2(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer2 size = %lx (mode %d, gs.path2.tag.nloop = %d).", size, gs.path[1].mode, gs.path[1].tag.nloop);

	_GSgifTransfer<1>(pMem, size);
}

void CALLBACK GSgifTransfer3(u32 *pMem, u32 size)
{
	FUNCLOG

	//ZZLog::GS_Log("GSgifTransfer3 size = %lx (mode %d, gs.path3.tag.nloop = %d).", size, gs.path[2].mode, gs.path[2].tag.nloop);

	nPath3Hack = 0;
	_GSgifTransfer<2>(pMem, size);
}

#endif
