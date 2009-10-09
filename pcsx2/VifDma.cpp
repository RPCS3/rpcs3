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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "VifDma.h"
#include "VifDma_internal.h"
#include "VUmicro.h"
#include "Tags.h"

#include <xmmintrin.h>
#include <emmintrin.h>

// Extern variables
extern "C"
{
	// Need cdecl on these for ASM references.
	extern VIFregisters *vifRegs;
	extern u32* vifMaskRegs;
	extern u32* vifRow;
}

extern vifStruct *vif;

int g_vifCycles = 0;
u8 s_maskwrite[256];

/* block size; data size; group size; qword size; */
#define _UNPACK_TABLE32(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name,         UNPACK_##name, \
	 bsize, dsize, gsize, qsize },

#define _UNPACK_TABLE(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name##u,      UNPACK_##name##s, \
	 bsize, dsize, gsize, qsize },

// Main table for function unpacking
const VIFUnpackFuncTable VIFfuncTable[16] =
{
	_UNPACK_TABLE32(S_32, 1, 4, 4, 4)		// 0x0 - S-32
	_UNPACK_TABLE(S_16, 2, 2, 2, 4)			// 0x1 - S-16
	_UNPACK_TABLE(S_8, 4, 1, 1, 4)			// 0x2 - S-8
	{
		NULL, NULL, 0, 0, 0, 0
	}
	,				// 0x3

	_UNPACK_TABLE32(V2_32, 24, 4, 8, 2)		// 0x4 - V2-32
	_UNPACK_TABLE(V2_16, 12, 2, 4, 2)		// 0x5 - V2-16
	_UNPACK_TABLE(V2_8, 6, 1, 2, 2)			// 0x6 - V2-8
	{
		NULL, NULL, 0, 0, 0, 0
	}
	,	// 0x7

	_UNPACK_TABLE32(V3_32, 36, 4, 12, 3)	// 0x8 - V3-32
	_UNPACK_TABLE(V3_16, 18, 2, 6, 3)		// 0x9 - V3-16
	_UNPACK_TABLE(V3_8, 9, 1, 3, 3)			// 0xA - V3-8
	{
		NULL, NULL, 0, 0, 0, 0
	}
	,				// 0xB

	_UNPACK_TABLE32(V4_32, 48, 4, 16, 4)	// 0xC - V4-32
	_UNPACK_TABLE(V4_16, 24, 2, 8, 4)		// 0xD - V4-16
	_UNPACK_TABLE(V4_8, 12, 1, 4, 4)		// 0xE - V4-8
	_UNPACK_TABLE32(V4_5, 6, 2, 2, 4)		// 0xF - V4-5
};

struct VIFSSEUnpackTable
{
	// regular 0, 1, 2; mask 0, 1, 2
	UNPACKPARTFUNCTYPESSE       funcU[9], funcS[9];
};

#define DECL_UNPACK_TABLE_SSE(name, sign) \
extern "C" { \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_0(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_1(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Regular_2(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_0(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_1(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_Mask_2(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_0(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_1(u32* dest, u32* data, int dmasize); \
	extern int UNPACK_SkippingWrite_##name##_##sign##_WriteMask_2(u32* dest, u32* data, int dmasize); \
}

#define _UNPACK_TABLE_SSE(name, sign) \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_0, \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_1, \
	UNPACK_SkippingWrite_##name##_##sign##_Regular_2, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_0, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_1, \
	UNPACK_SkippingWrite_##name##_##sign##_Mask_2, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_0, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_1, \
	UNPACK_SkippingWrite_##name##_##sign##_WriteMask_2 \

#define _UNPACK_TABLE_SSE_NULL \
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL

// Main table for function unpacking
DECL_UNPACK_TABLE_SSE(S_32, u);
DECL_UNPACK_TABLE_SSE(S_16, u);
DECL_UNPACK_TABLE_SSE(S_8, u);
DECL_UNPACK_TABLE_SSE(S_16, s);
DECL_UNPACK_TABLE_SSE(S_8, s);

DECL_UNPACK_TABLE_SSE(V2_32, u);
DECL_UNPACK_TABLE_SSE(V2_16, u);
DECL_UNPACK_TABLE_SSE(V2_8, u);
DECL_UNPACK_TABLE_SSE(V2_16, s);
DECL_UNPACK_TABLE_SSE(V2_8, s);

DECL_UNPACK_TABLE_SSE(V3_32, u);
DECL_UNPACK_TABLE_SSE(V3_16, u);
DECL_UNPACK_TABLE_SSE(V3_8, u);
DECL_UNPACK_TABLE_SSE(V3_16, s);
DECL_UNPACK_TABLE_SSE(V3_8, s);

DECL_UNPACK_TABLE_SSE(V4_32, u);
DECL_UNPACK_TABLE_SSE(V4_16, u);
DECL_UNPACK_TABLE_SSE(V4_8, u);
DECL_UNPACK_TABLE_SSE(V4_16, s);
DECL_UNPACK_TABLE_SSE(V4_8, s);
DECL_UNPACK_TABLE_SSE(V4_5, u);

static const VIFSSEUnpackTable VIFfuncTableSSE[16] =
{
	{ _UNPACK_TABLE_SSE(S_32, u), _UNPACK_TABLE_SSE(S_32, u) },
	{ _UNPACK_TABLE_SSE(S_16, u), _UNPACK_TABLE_SSE(S_16, s) },
	{ _UNPACK_TABLE_SSE(S_8, u), _UNPACK_TABLE_SSE(S_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL  },

	{ _UNPACK_TABLE_SSE(V2_32, u), _UNPACK_TABLE_SSE(V2_32, u) },
	{ _UNPACK_TABLE_SSE(V2_16, u), _UNPACK_TABLE_SSE(V2_16, s) },
	{ _UNPACK_TABLE_SSE(V2_8, u), _UNPACK_TABLE_SSE(V2_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL },

	{ _UNPACK_TABLE_SSE(V3_32, u), _UNPACK_TABLE_SSE(V3_32, u) },
	{ _UNPACK_TABLE_SSE(V3_16, u), _UNPACK_TABLE_SSE(V3_16, s) },
	{ _UNPACK_TABLE_SSE(V3_8, u), _UNPACK_TABLE_SSE(V3_8, s) },
	{ _UNPACK_TABLE_SSE_NULL, _UNPACK_TABLE_SSE_NULL },

	{ _UNPACK_TABLE_SSE(V4_32, u), _UNPACK_TABLE_SSE(V4_32, u) },
	{ _UNPACK_TABLE_SSE(V4_16, u), _UNPACK_TABLE_SSE(V4_16, s) },
	{ _UNPACK_TABLE_SSE(V4_8, u), _UNPACK_TABLE_SSE(V4_8, s) },
	{ _UNPACK_TABLE_SSE(V4_5, u), _UNPACK_TABLE_SSE(V4_5, u) },
};

void vifDmaInit()
{
}

template void ProcessMemSkip<0>(u32 size, u32 unpackType);
template void ProcessMemSkip<1>(u32 size, u32 unpackType);
template<const u32 VIFdmanum> void ProcessMemSkip(u32 size, u32 unpackType)
{
	const VIFUnpackFuncTable *unpack;

	unpack = &VIFfuncTable[ unpackType ];

	switch (unpackType)
	{
		case 0x0:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing S-32 skip, size = %d", size);
			break;
		case 0x1:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing S-16 skip, size = %d", size);
			break;
		case 0x2:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing S-8 skip, size = %d", size);
			break;
		case 0x4:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V2-32 skip, size = %d", size);
			break;
		case 0x5:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V2-16 skip, size = %d", size);
			break;
		case 0x6:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V2-8 skip, size = %d", size);
			break;
		case 0x8:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V3-32 skip, size = %d", size);
			break;
		case 0x9:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V3-16 skip, size = %d", size);
			break;
		case 0xA:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V3-8 skip, size = %d", size);
			break;
		case 0xC:
			vif->tag.addr += size;
			VIFUNPACK_LOG("Processing V4-32 skip, size = %d, CL = %d, WL = %d", size, vifRegs->cycle.cl, vifRegs->cycle.wl);
			break;
		case 0xD:
		    vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V4-16 skip, size = %d", size);
			break;
		case 0xE:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V4-8 skip, size = %d", size);
			break;
		case 0xF:
			vif->tag.addr += (size / unpack->gsize) * 16;
			VIFUNPACK_LOG("Processing V4-5 skip, size = %d", size);
			break;
		default:
			Console.WriteLn("Invalid unpack type %x", unpackType);
			break;
	}

	//Append any skips in to the equation

	if (vifRegs->cycle.cl > vifRegs->cycle.wl)
	{
		VIFUNPACK_LOG("Old addr %x CL %x WL %x", vif->tag.addr, vifRegs->cycle.cl, vifRegs->cycle.wl);
		vif->tag.addr += (size / (unpack->gsize*vifRegs->cycle.wl)) * ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);
		VIFUNPACK_LOG("New addr %x CL %x WL %x", vif->tag.addr, vifRegs->cycle.cl, vifRegs->cycle.wl);
	}

	//This is sorted out later
	if ((vif->tag.addr & 0xf) != (vifRegs->offset * 4))
	{
		VIFUNPACK_LOG("addr aligned to %x", vif->tag.addr);
		vif->tag.addr = (vif->tag.addr & ~0xf) + (vifRegs->offset * 4);
	}
	
	if (vif->tag.addr >= (u32)vif_size(VIFdmanum))
	{
		vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
	}
}

template u32 VIFalign<0>(u32 *data, vifCode *v, u32 size);
template u32 VIFalign<1>(u32 *data, vifCode *v, u32 size);
template<const u32 VIFdmanum> u32 VIFalign(u32 *data, vifCode *v, u32 size)
{
	u32 *dest;
	u32 unpackType;
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	VURegs * VU;
	u8 *cdata = (u8*)data;

#ifdef PCSX2_DEBUG
	u32 memsize = vif_size(VIFdmanum);
#endif

	if (VIFdmanum == 0)
	{
		VU = &VU0;
		vifRegs = vif0Regs;
		vifMaskRegs = g_vif0Masks;
		vif = &vif0;
		vifRow = g_vifmask.Row0;
	}
	else
	{
		VU = &VU1;
		vifRegs = vif1Regs;
		vifMaskRegs = g_vif1Masks;
		vif = &vif1;
		vifRow = g_vifmask.Row1;
	}
	assert(v->addr < memsize);

	dest = (u32*)(VU->Mem + v->addr);

	VIF_LOG("VIF%d UNPACK Align: Mode=%x, v->size=%d, size=%d, v->addr=%x v->num=%x",
	        VIFdmanum, v->cmd & 0xf, v->size, size, v->addr, vifRegs->num);

	// The unpack type
	unpackType = v->cmd & 0xf;

	ft = &VIFfuncTable[ unpackType ];
	func = vif->usn ? ft->funcU : ft->funcS;

	size <<= 2;

#ifdef PCSX2_DEBUG
	memsize = size;
#endif

	if(vifRegs->offset != 0)
	{
		int unpacksize;

		//This is just to make sure the alignment isn't loopy on a split packet
		if(vifRegs->offset != ((vif->tag.addr & 0xf) >> 2))
		{
			DevCon.Error("Warning: Unpack alignment error");
		}

		VIFUNPACK_LOG("Aligning packet size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);

		if (((u32)size / (u32)ft->dsize) < ((u32)ft->qsize - vifRegs->offset))
		{
				DevCon.Error("Wasn't enough left size/dsize = %x left to write %x", (size / ft->dsize), (ft->qsize - vifRegs->offset));
		}
		unpacksize = min((size / ft->dsize), (ft->qsize - vifRegs->offset));


		VIFUNPACK_LOG("Increasing dest by %x from offset %x", (4 - ft->qsize) + unpacksize, vifRegs->offset);

		func(dest, (u32*)cdata, unpacksize);
		size -= unpacksize * ft->dsize;

		if(vifRegs->offset == 0)
		{
			vifRegs->num--;
			++vif->cl;
		}
		else
		{
			DevCon.Notice("Offset = %x", vifRegs->offset);
			vif->tag.addr += unpacksize * 4;
			return size>>2;
		}

		if (vif->cl == vifRegs->cycle.wl)
		{
			if (vifRegs->cycle.cl != vifRegs->cycle.wl)
			{
				vif->tag.addr += (((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + ((4 - ft->qsize) + unpacksize)) * 4;
				dest += ((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + (4 - ft->qsize) + unpacksize;
			}
			else
			{
				vif->tag.addr += ((4 - ft->qsize) + unpacksize) * 4;
				dest += (4 - ft->qsize) + unpacksize;
			}

			if (vif->tag.addr >= (u32)vif_size(VIFdmanum))
			{
				vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
				dest = (u32*)(VU->Mem + v->addr);
			}

			cdata += unpacksize * ft->dsize;
			vif->cl = 0;
			VIFUNPACK_LOG("Aligning packet done size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);
			if ((size & 0xf) == 0) return size >> 2;

		}
		else
		{
			vif->tag.addr += ((4 - ft->qsize) + unpacksize) * 4;
			dest += (4 - ft->qsize) + unpacksize;

			if (vif->tag.addr >= (u32)vif_size(VIFdmanum))
			{
				vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
				dest = (u32*)(VU->Mem + v->addr);
			}

			cdata += unpacksize * ft->dsize;
			VIFUNPACK_LOG("Aligning packet done size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);
		}
	}

	if (vif->cl != 0 || (size & 0xf))  //Check alignment for SSE unpacks
	{
		int incdest;

		if (vifRegs->cycle.cl >= vifRegs->cycle.wl)  // skipping write
		{
			if (vif->tag.addr >= (u32)vif_size(VIFdmanum))
			{
				vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
				dest = (u32*)(VU->Mem + v->addr);
			}
			// continuation from last stream
			VIFUNPACK_LOG("Continuing last stream size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);
			incdest = ((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + 4;

			while ((size >= ft->gsize) && (vifRegs->num > 0))
			{
				func(dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				size -= ft->gsize;

				vifRegs->num--;
				++vif->cl;
				if (vif->cl == vifRegs->cycle.wl)
				{
					dest += incdest;
					vif->tag.addr += incdest * 4;

					vif->cl = 0;
					if ((size & 0xf) == 0) break;
				}
				else
				{
					dest += 4;
					vif->tag.addr += 16;
				}

				if (vif->tag.addr >= (u32)vif_size(VIFdmanum))
				{
					vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
					dest = (u32*)(VU->Mem + v->addr);
				}
			}

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow = VIFdmanum ? g_vifmask.Row1 : g_vifmask.Row0;
				vifRow[0] = vifRegs->r0;
				vifRow[1] = vifRegs->r1;
				vifRow[2] = vifRegs->r2;
				vifRow[3] = vifRegs->r3;
			}

		}
		if (size >= ft->dsize && vifRegs->num > 0 && ((size & 0xf) != 0 || vif->cl != 0))
		{
			//VIF_LOG("warning, end with size = %d", size);
			/* unpack one qword */
			if(vif->tag.addr + ((size / ft->dsize) * 4)  >= (u32)vif_size(VIFdmanum))
			{
				//DevCon.Notice("Overflow");
				vif->tag.addr &= (u32)(vif_size(VIFdmanum) - 1);
				dest = (u32*)(VU->Mem + v->addr);
			}

			vif->tag.addr += (size / ft->dsize) * 4;

			func(dest, (u32*)cdata, size / ft->dsize);
			size = 0;

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow[0] = vifRegs->r0;
				vifRow[1] = vifRegs->r1;
				vifRow[2] = vifRegs->r2;
				vifRow[3] = vifRegs->r3;
			}
			VIFUNPACK_LOG("leftover done, size %d, vifnum %d, addr %x", size, vifRegs->num, vif->tag.addr);
		}
	}
	return size>>2;
}

template void VIFunpack<0>(u32 *data, vifCode *v, u32 size);
template void VIFunpack<1>(u32 *data, vifCode *v, u32 size);
template<const u32 VIFdmanum> void VIFunpack(u32 *data, vifCode *v, u32 size)
{
	u32 *dest;
	u32 unpackType;
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	VURegs * VU;
	u8 *cdata = (u8*)data;
	u32 tempsize = 0;
	const u32 memlimit = vif_size(VIFdmanum);

#ifdef PCSX2_DEBUG
	u32 memsize = memlimit;
#endif

	_mm_prefetch((char*)data, _MM_HINT_NTA);

	if (VIFdmanum == 0)
	{
		VU = &VU0;
		vifRegs = vif0Regs;
		vifMaskRegs = g_vif0Masks;
		vif = &vif0;
		vifRow = g_vifmask.Row0;
		assert(v->addr < memsize);
	}
	else
	{

		VU = &VU1;
		vifRegs = vif1Regs;
		vifMaskRegs = g_vif1Masks;
		vif = &vif1;
		vifRow = g_vifmask.Row1;
		assert(v->addr < memsize);
	}

	dest = (u32*)(VU->Mem + v->addr);

	VIF_LOG("VIF%d UNPACK: Mode=%x, v->size=%d, size=%d, v->addr=%x v->num=%x",
	        VIFdmanum, v->cmd & 0xf, v->size, size, v->addr, vifRegs->num);

	VIFUNPACK_LOG("USN %x Masking %x Mask %x Mode %x CL %x WL %x Offset %x", vif->usn, (vifRegs->code & 0x10000000) >> 28, vifRegs->mask, vifRegs->mode, vifRegs->cycle.cl, vifRegs->cycle.wl, vifRegs->offset);

	// The unpack type
	unpackType = v->cmd & 0xf;

	_mm_prefetch((char*)data + 128, _MM_HINT_NTA);

	ft = &VIFfuncTable[ unpackType ];
	func = vif->usn ? ft->funcU : ft->funcS;

	size <<= 2;

#ifdef PCSX2_DEBUG
	memsize = size;
#endif

	if (vifRegs->cycle.cl >= vifRegs->cycle.wl)   // skipping write
	{
		if (v->addr >= memlimit)
		{
			//DevCon.Notice("Overflown at the start");
			v->addr &= (memlimit - 1);
			dest = (u32*)(VU->Mem + v->addr);
		}

		size = min(size, (int)vifRegs->num * ft->gsize); //size will always be the same or smaller

		tempsize = vif->tag.addr + ((((vifRegs->num-1) / vifRegs->cycle.wl) *
			(vifRegs->cycle.cl - vifRegs->cycle.wl)) * 16) + (vifRegs->num * 16);

		/*tempsize = vif->tag.addr + (((size / (ft->gsize * vifRegs->cycle.wl)) *
			(vifRegs->cycle.cl - vifRegs->cycle.wl)) * 16) + (vifRegs->num * 16);*/

		//Sanity Check (memory overflow)
		if (tempsize > memlimit)
		{
			if (((vifRegs->cycle.cl != vifRegs->cycle.wl) &&
			       ((memlimit + (vifRegs->cycle.cl - vifRegs->cycle.wl) * 16) == tempsize) ||
			       (tempsize == memlimit)))
			{
				//It's a red herring, so ignore it! SSE unpacks will be much quicker.
				tempsize = 0;
			}
			else
			{
				//DevCon.Notice("VIF%x Unpack ending %x > %x", VIFdmanum, tempsize, VIFdmanum ? 0x4000 : 0x1000);
				tempsize = size;
				size = 0;
			}
		}
		else
		{
#ifndef NON_SSE_UNPACKS
			tempsize = 0;
#else
			tempsize = size;
			size = 0;
#endif
		}

		if (size >= ft->gsize)
		{
			const UNPACKPARTFUNCTYPESSE* pfn;
			int writemask;
			u32 oldcycle = -1;

			// yay evil .. let's just set some XMM registers in the middle of C code
			// and "hope" they get preserved, in spite of the fact that x86-32 ABI specifies
			// these as "clobberable" registers (so any printf or something could decide to
			// clobber them, and has every right to... >_<) --air

#ifdef _MSC_VER
			if (VIFdmanum)
			{
				__asm movaps XMM_ROW, xmmword ptr [g_vifmask.Row1]
				__asm movaps XMM_COL, xmmword ptr [g_vifmask.Col1]
			}
			else
			{
				__asm movaps XMM_ROW, xmmword ptr [g_vifmask.Row0]
				__asm movaps XMM_COL, xmmword ptr [g_vifmask.Col0]
			}
#else
			// I'd add volatile to these, but what's the point?  This code already breaks
			// like 5000 coveted rules of binary interfacing regardless, and is only working by
			// the miracles and graces of a profound deity (or maybe it doesn't -- linux port
			// *does* have stability issues, especially in GCC 4.4). --air
			if (VIFdmanum)
			{
				__asm__(".intel_syntax noprefix\n"
				        "movaps xmm6, xmmword ptr [%[Row1]]\n"
				        "movaps xmm7, xmmword ptr [%[Col1]]\n"
					".att_syntax\n" : : [Row1]"r"(g_vifmask.Row1), [Col1]"r"(g_vifmask.Col1));
			}
			else
			{
				__asm__(".intel_syntax noprefix\n"
				        "movaps xmm6, xmmword ptr [%[Row0]]\n"
				        "movaps xmm7, xmmword ptr [%[Col0]]\n"
					".att_syntax\n" : : [Row0]"r"(g_vifmask.Row0),  [Col0]"r"(g_vifmask.Col0));
			}
#endif

			if ((vifRegs->cycle.cl == 0) || (vifRegs->cycle.wl == 0) ||
			   ((vifRegs->cycle.cl == vifRegs->cycle.wl) && !(vifRegs->code & 0x10000000)))
			{
				oldcycle = *(u32*) & vifRegs->cycle;
				vifRegs->cycle.cl = vifRegs->cycle.wl = 1;
			}

			pfn = vif->usn ? VIFfuncTableSSE[unpackType].funcU : VIFfuncTableSSE[unpackType].funcS;
			writemask = VIFdmanum ? g_vif1HasMask3[min(vifRegs->cycle.wl,(u8)3)] : g_vif0HasMask3[min(vifRegs->cycle.wl,(u8)3)];
			writemask = pfn[(((vifRegs->code & 0x10000000)>>28)<<writemask)*3+vifRegs->mode](dest, (u32*)cdata, size);

			if (oldcycle != -1) *(u32*)&vifRegs->cycle = oldcycle;

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for non SSE
				vifRegs->r0 = vifRow[0];
				vifRegs->r1 = vifRow[1];
				vifRegs->r2 = vifRow[2];
				vifRegs->r3 = vifRow[3];
			}

			// if size is left over, update the src,dst pointers
			if (writemask > 0)
			{
				int left = (size - writemask) / ft->gsize;
				cdata += left * ft->gsize;
				dest = (u32*)((u8*)dest + ((left / vifRegs->cycle.wl) * vifRegs->cycle.cl + left % vifRegs->cycle.wl) * 16);
				vifRegs->num -= left;
				vif->cl = (size % (ft->gsize * vifRegs->cycle.wl)) / ft->gsize;
				size = writemask;

				if (size >= ft->dsize && vifRegs->num > 0)
				{
					VIF_LOG("warning, end with size = %d", size);

					/* unpack one qword */
					//vif->tag.addr += (size / ft->dsize) * 4;
					func(dest, (u32*)cdata, size / ft->dsize);
					size = 0;

					if(vifRegs->mode == 2)
					{
						//Update the reg rows for SSE
						vifRow[0] = vifRegs->r0;
						vifRow[1] = vifRegs->r1;
						vifRow[2] = vifRegs->r2;
						vifRow[3] = vifRegs->r3;
					}
					VIFUNPACK_LOG("leftover done, size %d, vifnum %d, addr %x", size, vifRegs->num, vif->tag.addr);
				}
			}
			else
			{
				vifRegs->num -= size / ft->gsize;
				if (vifRegs->num > 0) vif->cl = (size % (ft->gsize * vifRegs->cycle.wl)) / ft->gsize;
				size = 0;
			}
		}
		else if(tempsize)
		{
			int incdest = ((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + 4;
			size = 0;
			int addrstart = v->addr;
			if((tempsize >> 2) != vif->tag.size) DevCon.Notice("split when size != tagsize");

			VIFUNPACK_LOG("sorting tempsize :p, size %d, vifnum %d, addr %x", tempsize, vifRegs->num, vif->tag.addr);

			while ((tempsize >= ft->gsize) && (vifRegs->num > 0))
			{
				if(v->addr >= memlimit)
				{
					DevCon.Notice("Mem limit overflow");
					v->addr &= (memlimit - 1);
					dest = (u32*)(VU->Mem + v->addr);
				}

				func(dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				tempsize -= ft->gsize;

				vifRegs->num--;
				++vif->cl;
				
				if (vif->cl == vifRegs->cycle.wl)
				{
					dest += incdest;
					v->addr += (incdest * 4);
					vif->cl = 0;
				}
				else
				{
					dest += 4;
					v->addr += 16;
				}
			}

			if (vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow[0] = vifRegs->r0;
				vifRow[1] = vifRegs->r1;
				vifRow[2] = vifRegs->r2;
				vifRow[3] = vifRegs->r3;
			}
			
			if (v->addr >= memlimit)
            {
                v->addr &= (memlimit - 1);
                dest = (u32*)(VU->Mem + v->addr);
            }
				
			v->addr = addrstart;
			if(tempsize > 0) size = tempsize;
		}
		
		if (size >= ft->dsize && vifRegs->num > 0) //Else write what we do have
		{
			VIF_LOG("warning, end with size = %d", size);

			/* unpack one qword */
			//vif->tag.addr += (size / ft->dsize) * 4;
			func(dest, (u32*)cdata, size / ft->dsize);
			size = 0;

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow[0] = vifRegs->r0;
				vifRow[1] = vifRegs->r1;
				vifRow[2] = vifRegs->r2;
				vifRow[3] = vifRegs->r3;
			}
			VIFUNPACK_LOG("leftover done, size %d, vifnum %d, addr %x", size, vifRegs->num, vif->tag.addr);
		}
	}
	else   /* filling write */
	{

		if(vifRegs->cycle.cl > 0) // Quicker and avoids zero division :P
			if((u32)(((size / ft->gsize) / vifRegs->cycle.cl) * vifRegs->cycle.wl) < vifRegs->num)
			DevCon.Notice("Filling write warning! %x < %x and CL = %x WL = %x", (size / ft->gsize), vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl);

		//DevCon.Notice("filling write %d cl %d, wl %d mask %x mode %x unpacktype %x addr %x", vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl, vifRegs->mask, vifRegs->mode, unpackType, vif->tag.addr);
		while (vifRegs->num > 0)
		{
			if (vif->cl == vifRegs->cycle.wl)
			{
				vif->cl = 0;
			}

			if (vif->cl < vifRegs->cycle.cl)   /* unpack one qword */
			{
				if(size < ft->gsize)
				{
					VIF_LOG("Out of Filling write data");
					break;
				}

				func(dest, (u32*)cdata, ft->qsize);
				cdata += ft->gsize;
				size -= ft->gsize;
				vif->cl++;
				vifRegs->num--;

				if (vif->cl == vifRegs->cycle.wl)
				{
					vif->cl = 0;
				}
			}
			else
			{
				func(dest, (u32*)cdata, ft->qsize);
				vif->tag.addr += 16;
				vifRegs->num--;
				++vif->cl;

			}
			dest += 4;
			if (vifRegs->num == 0) break;
		}
	}
}

template void vuExecMicro<0>(u32 addr);
template void vuExecMicro<1>(u32 addr);
template<const u32 VIFdmanum> void vuExecMicro(u32 addr)
{
	VURegs * VU;

	if (VIFdmanum == 0)
	{
		VU = &VU0;
		vif0FLUSH();
	}
	else
	{
		VU = &VU1;
		vif1FLUSH();
	}

	if (VU->vifRegs->itops > (VIFdmanum ? 0x3ffu : 0xffu))
		Console.WriteLn("VIF%d ITOP overrun! %x", VIFdmanum, VU->vifRegs->itops);

	VU->vifRegs->itop = VU->vifRegs->itops;

	if (VIFdmanum == 1)
	{
		/* in case we're handling a VIF1 execMicro, set the top with the tops value */
		VU->vifRegs->top = VU->vifRegs->tops & 0x3ff;

		/* is DBF flag set in VIF_STAT? */
		if (VU->vifRegs->stat.DBF)
		{
			/* it is, so set tops with base, and clear the stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base;
			VU->vifRegs->stat.DBF = 0;
		}
		else
		{
			/* it is not, so set tops with base + offset,  and set stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base + VU->vifRegs->ofst;
			VU->vifRegs->stat.DBF = 1;
		}
	}

	if (VIFdmanum == 0)
		vu0ExecMicro(addr);
	else
		vu1ExecMicro(addr);
}
