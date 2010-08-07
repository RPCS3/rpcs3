/*  GSnull
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


 // Processes a GIFtag & packet, and throws out some gsIRQs as needed.
// Used to keep interrupts in sync with the EE, while the GS itself
// runs potentially several frames behind.
// size - size of the packet in simd128's

#include "GS.h"
#include "GifTransfer.h"

using namespace std;

extern GSVars gs;

PCSX2_ALIGNED16( u8 g_RealGSMem[0x2000] );

template<int index> void _GSgifTransfer(const u32 *pMem, u32 size)
{
//	FUNCLOG

	pathInfo *path = &gs.path[index];
	
	while (size > 0)
	{
		//GS_LOG(_T("Transfer(%08x, %d) START\n"), pMem, size);
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
						GIFPackedRegHandlers[reg](pMem);
						
						pMem += 4; //sizeof(GIFPackedReg)/4;
						size--;
					}
					while (path->StepReg() && (size > 0));

					break;
				}

				case GIF_FLG_REGLIST:
				{
					//GS_Log("%8.8x%8.8x %d L", ((u32*)&gs.regs)[1], *(u32*)&gs.regs, path->tag.nreg/4);

					size *= 2;

					do
					{
						GIFRegHandlers[path->GetReg()](pMem);

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
					//GS_LOG("GIF_FLG_IMAGE(%d)=%d", gs.imageTransfer, len);

					switch (gs.imageTransfer)
					{
						case 0:
							//TransferHostLocal(pMem, len * 4);
							break;

						case 1:
							// This can't happen; downloads can not be started or performed as part of
							// a GIFtag operation.  They're an entirely separate process that can only be
							// done through the ReverseFIFO transfer (aka ReadFIFO). --air
							assert(0);
							break;

						case 2:
							// //TransferLocalLocal();
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
					GS_LOG("*** WARNING **** Unexpected GIFTag flag.");
					assert(0);
					path->nloop = 0;
					break;
			}
		}
	}
}

#define DO_GIF_TRANSFERS

// Obsolete. Included because it's still in GSdef.
EXPORT_C_(void) GSgifTransfer1(u32 *pMem, u32 addr)
{
#ifdef DO_GIF_TRANSFERS
	_GSgifTransfer<0>((u32*)((u8*)pMem + addr), (0x4000 - addr) / 16);
#endif
}

EXPORT_C_(void) GSgifTransfer(const u32 *pMem, u32 size)
{
#ifdef DO_GIF_TRANSFERS
	_GSgifTransfer<3>(const_cast<u32*>(pMem), size);
#endif
}

EXPORT_C_(void) GSgifTransfer2(u32 *pMem, u32 size)
{
#ifdef DO_GIF_TRANSFERS
	_GSgifTransfer<1>(const_cast<u32*>(pMem), size);
#endif
}

EXPORT_C_(void) GSgifTransfer3(u32 *pMem, u32 size)
{
#ifdef DO_GIF_TRANSFERS
	_GSgifTransfer<2>(const_cast<u32*>(pMem), size);
#endif
}

void InitPath()
{
	gs.path[0].mode = gs.path[1].mode = gs.path[2].mode = gs.path[3].mode = 0;
}

