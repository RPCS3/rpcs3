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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "Vif.h"
#include "VUmicro.h"
#include "GS.h"
#include "VifDma.h"

#include <xmmintrin.h>
#include <emmintrin.h>

using namespace std;			// for min / max

// Extern variables
extern "C"
{
	// Need cdecl on these for ASM references.
	extern VIFregisters *vifRegs;
	extern u32* vifMaskRegs;
	extern u32* vifRow;
}

PCSX2_ALIGNED16_EXTERN(u32 g_vifRow0[4]);
PCSX2_ALIGNED16_EXTERN(u32 g_vifCol0[4]);
PCSX2_ALIGNED16_EXTERN(u32 g_vifRow1[4]);
PCSX2_ALIGNED16_EXTERN(u32 g_vifCol1[4]);

extern vifStruct *vif;

vifStruct vif0, vif1;

static PCSX2_ALIGNED16(u32 g_vif1Masks[64]);
static PCSX2_ALIGNED16(u32 g_vif0Masks[64]);
u32 g_vif1HasMask3[4] = {0}, g_vif0HasMask3[4] = {0};

// Generic constants
static const unsigned int VIF0intc = 4;
static const unsigned int VIF1intc = 5;
static const unsigned int VIF0dmanum = 0;
static const unsigned int VIF1dmanum = 1;

int g_vifCycles = 0;
int Path3progress = 2; //0 = Image Mode (DirectHL), 1 = transferring, 2 = Stopped at End of Packet

u32 splittransfer[4];
u32 splitptr = 0;

typedef void (__fastcall *UNPACKFUNCTYPE)(u32 *dest, u32 *data, int size);
typedef int (*UNPACKPARTFUNCTYPESSE)(u32 *dest, u32 *data, int size);
extern void (*Vif1CMDTLB[82])();
extern void (*Vif0CMDTLB[75])();
extern int (__fastcall *Vif1TransTLB[128])(u32 *data);
extern int (__fastcall *Vif0TransTLB[128])(u32 *data);

u32 *vif0ptag, *vif1ptag;
u8 s_maskwrite[256];

struct VIFUnpackFuncTable
{
	UNPACKFUNCTYPE       funcU;
	UNPACKFUNCTYPE       funcS;

	u32 bsize; // currently unused
	u32 dsize; // byte size of one channel
	u32 gsize; // size of data in bytes used for each write cycle
	u32 qsize; // used for unpack parts, num of vectors that
	// will be decompressed from data for 1 cycle
};

/* block size; data size; group size; qword size; */
#define _UNPACK_TABLE32(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name,         UNPACK_##name, \
	 bsize, dsize, gsize, qsize },

#define _UNPACK_TABLE(name, bsize, dsize, gsize, qsize) \
   { UNPACK_##name##u,      UNPACK_##name##s, \
	 bsize, dsize, gsize, qsize },

// Main table for function unpacking
static const VIFUnpackFuncTable VIFfuncTable[16] =
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

__forceinline void vif0FLUSH()
{
	int _cycles = VU0.cycle;
	
	// fixme: this code should call _vu0WaitMicro instead?  I'm not sure if
	// it's purposefully ignoring ee cycles or not (see below for more)

	vu0Finish();
	g_vifCycles += (VU0.cycle - _cycles) * BIAS;
}

__forceinline void vif1FLUSH()
{
	int _cycles = VU1.cycle;

	// fixme: Same as above, is this a "stalling" offense?  I think the cycles should
	// be added to cpuRegs.cycle instead of g_vifCycles, but not sure (air)

	if (VU0.VI[REG_VPU_STAT].UL & 0x100)
	{
		do
		{
			CpuVU1.ExecuteBlock();
		}
		while (VU0.VI[REG_VPU_STAT].UL & 0x100);

		g_vifCycles += (VU1.cycle - _cycles) * BIAS;
	}
}

void vifDmaInit()
{
}

__forceinline static int _limit(int a, int max)
{
	return (a > max ? max : a);
}

static void ProcessMemSkip(int size, unsigned int unpackType, const unsigned int VIFdmanum)
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
			Console::WriteLn("Invalid unpack type %x", params unpackType);
			break;
	}

	//Append any skips in to the equasion
	
	if (vifRegs->cycle.cl > vifRegs->cycle.wl)
	{
		VIFUNPACK_LOG("Old addr %x CL %x WL %x", vif->tag.addr, vifRegs->cycle.cl, vifRegs->cycle.wl);
		vif->tag.addr += (size / (unpack->gsize*vifRegs->cycle.wl)) * ((vifRegs->cycle.cl - vifRegs->cycle.wl)*16);
		VIFUNPACK_LOG("New addr %x CL %x WL %x", vif->tag.addr, vifRegs->cycle.cl, vifRegs->cycle.wl);
	}

	//This is sorted out later
	if((vif->tag.addr & 0xf) != (vifRegs->offset * 4))
	{
		VIFUNPACK_LOG("addr aligned to %x", vif->tag.addr);
		vif->tag.addr = (vif->tag.addr & ~0xf) + (vifRegs->offset * 4);
	}
	if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
	{
		vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
	}
	
}

static int VIFalign(u32 *data, vifCode *v, unsigned int size, const unsigned int VIFdmanum)
{
	u32 *dest;
	u32 unpackType;
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	VURegs * VU;
	u8 *cdata = (u8*)data;

#ifdef PCSX2_DEBUG
	u32 memsize = VIFdmanum ? 0x4000 : 0x1000;
#endif

	if (VIFdmanum == 0)
	{
		VU = &VU0;
		vifRegs = vif0Regs;
		vifMaskRegs = g_vif0Masks;
		vif = &vif0;
		vifRow = g_vifRow0;
	}
	else
	{
		VU = &VU1;
		vifRegs = vif1Regs;
		vifMaskRegs = g_vif1Masks;
		vif = &vif1;
		vifRow = g_vifRow1;
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

		//This is just to make sure the alignment isnt loopy on a split packet
		if(vifRegs->offset != ((vif->tag.addr & 0xf) >> 2))
		{
			DevCon::Error("Warning: Unpack alignment error");
		}

		VIFUNPACK_LOG("Aligning packet size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);

		if (((u32)size / (u32)ft->dsize) < ((u32)ft->qsize - vifRegs->offset))
		{
				DevCon::Error("Wasn't enough left size/dsize = %x left to write %x", params(size / ft->dsize), (ft->qsize - vifRegs->offset));
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
			DevCon::Notice("Offset = %x", params vifRegs->offset);
			vif->tag.addr += unpacksize * 4;
			return size>>2;
		}

		if (vif->cl == vifRegs->cycle.wl)
		{
			if (vifRegs->cycle.cl != vifRegs->cycle.wl)
			{
				vif->tag.addr += (((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + ((4 - ft->qsize) + unpacksize)) * 4;
				dest += ((vifRegs->cycle.cl - vifRegs->cycle.wl) << 2) + (4 - ft->qsize) + unpacksize;
				if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
				{
					vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
					dest = (u32*)(VU->Mem + v->addr);
				}
			}
			else
			{
				vif->tag.addr += ((4 - ft->qsize) + unpacksize) * 4;
				dest += (4 - ft->qsize) + unpacksize;
				if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
				{
					vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
					dest = (u32*)(VU->Mem + v->addr);
				}
				
			}
			cdata += unpacksize * ft->dsize;
			vif->cl = 0;
			VIFUNPACK_LOG("Aligning packet done size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);
			if((size & 0xf) == 0)return size >> 2;

		}
		else
		{
			vif->tag.addr += ((4 - ft->qsize) + unpacksize) * 4;
			dest += (4 - ft->qsize) + unpacksize;		
			if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
			{
				vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
				dest = (u32*)(VU->Mem + v->addr);
			}
			cdata += unpacksize * ft->dsize;
			VIFUNPACK_LOG("Aligning packet done size = %d offset %d addr %x", size, vifRegs->offset, vif->tag.addr);
		}
	}

	if (vif->cl != 0 || (size & 0xf))  //Check alignment for SSE unpacks
	{

#ifdef PCSX2_DEBUG
		static int s_count = 0;
#endif

		int incdest;

		if (vifRegs->cycle.cl >= vifRegs->cycle.wl)  // skipping write
		{
			if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
			{
				vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
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
					if((size & 0xf) == 0)break;
				}
				else
				{
					dest += 4;
					vif->tag.addr += 16;
				}
				if(vif->tag.addr >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
				{
					vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
					dest = (u32*)(VU->Mem + v->addr);
				}
			}

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow = VIFdmanum ? g_vifRow1 : g_vifRow0;
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
			if(vif->tag.addr + ((size / ft->dsize) * 4)  >= (u32)(VIFdmanum ? 0x4000 : 0x1000)) 
			{
				//DevCon::Notice("Overflow");
				vif->tag.addr &= (u32)(VIFdmanum ? 0x3fff : 0xfff);
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

static void VIFunpack(u32 *data, vifCode *v, unsigned int size, const unsigned int VIFdmanum)
{
	u32 *dest;
	u32 unpackType;
	UNPACKFUNCTYPE func;
	const VIFUnpackFuncTable *ft;
	VURegs * VU;
	u8 *cdata = (u8*)data;
	u32 tempsize = 0;
	const u32 memlimit = (VIFdmanum ? 0x4000 : 0x1000);
	
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
		vifRow = g_vifRow0;
		assert(v->addr < memsize);
	}
	else
	{

		VU = &VU1;
		vifRegs = vif1Regs;
		vifMaskRegs = g_vif1Masks;
		vif = &vif1;
		vifRow = g_vifRow1;
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

#ifdef PCSX2_DEBUG
		static int s_count = 0;
#endif
		if (v->addr >= memlimit) 
		{
			//DevCon::Notice("Overflown at the start");
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
				//Its a red herring! so ignore it! SSE unpacks will be much quicker
				tempsize = 0;
			}
			else
			{
				//DevCon::Notice("VIF%x Unpack ending %x > %x", params VIFdmanum, tempsize, VIFdmanum ? 0x4000 : 0x1000);
				tempsize = size;
				size = 0;
			}
		} 
		else 
		{
			tempsize = 0;  //Commenting out this then
			//tempsize = size; // -\_uncommenting these Two enables non-SSE unpacks
			//size = 0;		 // -/  
		}

		if (size >= ft->gsize)
		{
			const UNPACKPARTFUNCTYPESSE* pfn;
			int writemask;
			u32 oldcycle = -1;

#ifdef _MSC_VER
			if (VIFdmanum)
			{
				__asm movaps XMM_ROW, xmmword ptr [g_vifRow1]
				__asm movaps XMM_COL, xmmword ptr [g_vifCol1]
			}
			else
			{
				__asm movaps XMM_ROW, xmmword ptr [g_vifRow0]
				__asm movaps XMM_COL, xmmword ptr [g_vifCol0]
			}
#else
			if (VIFdmanum)
			{
				__asm__(".intel_syntax noprefix\n"
				        "movaps xmm6, xmmword ptr [%[g_vifRow1]]\n"
				        "movaps xmm7, xmmword ptr [%[g_vifCol1]]\n"
					".att_syntax\n" : : [g_vifRow1]"r"(g_vifRow1), [g_vifCol1]"r"(g_vifCol1));
			}
			else
			{
				__asm__(".intel_syntax noprefix\n"
				        "movaps xmm6, xmmword ptr [%[g_vifRow0]]\n"
				        "movaps xmm7, xmmword ptr [%[g_vifCol0]]\n"
					".att_syntax\n" : : [g_vifRow0]"r"(g_vifRow0),  [g_vifCol0]"r"(g_vifCol0));
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
					//VIF_LOG("warning, end with size = %d", size);

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
			if((tempsize >> 2) != vif->tag.size) DevCon::Notice("split when size != tagsize");
			
			VIFUNPACK_LOG("sorting tempsize :p, size %d, vifnum %d, addr %x", tempsize, vifRegs->num, vif->tag.addr);

			while ((tempsize >= ft->gsize) && (vifRegs->num > 0))
			{
				if(v->addr >= memlimit) 
				{
					DevCon::Notice("Mem limit ovf");
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

			if(vifRegs->mode == 2)
			{
				//Update the reg rows for SSE
				vifRow[0] = vifRegs->r0;
				vifRow[1] = vifRegs->r1;
				vifRow[2] = vifRegs->r2;
				vifRow[3] = vifRegs->r3;
			}
			if(v->addr >= memlimit) 
				{
					v->addr &= (memlimit - 1);
					dest = (u32*)(VU->Mem + v->addr);
				}
			v->addr = addrstart;
			if(tempsize > 0) size = tempsize;

		}
		if (size >= ft->dsize && vifRegs->num > 0) //Else write what we do have
		{
			//VIF_LOG("warning, end with size = %d", size);

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
			DevCon::Notice("Filling write warning! %x < %x and CL = %x WL = %x", params (size / ft->gsize), vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl);
				
		//DevCon::Notice("filling write %d cl %d, wl %d mask %x mode %x unpacktype %x addr %x", params vifRegs->num, vifRegs->cycle.cl, vifRegs->cycle.wl, vifRegs->mask, vifRegs->mode, unpackType, vif->tag.addr);
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

static void vuExecMicro(u32 addr, const u32 VIFdmanum)
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
		Console::WriteLn("VIF%d ITOP overrun! %x", params VIFdmanum, VU->vifRegs->itops);

	VU->vifRegs->itop = VU->vifRegs->itops;

	if (VIFdmanum == 1)
	{
		/* in case we're handling a VIF1 execMicro, set the top with the tops value */
		VU->vifRegs->top = VU->vifRegs->tops & 0x3ff;

		/* is DBF flag set in VIF_STAT? */
		if (VU->vifRegs->stat & 0x80)
		{
			/* it is, so set tops with base, and set the stat DBF flag */ 
			VU->vifRegs->tops = VU->vifRegs->base;
			VU->vifRegs->stat &= ~0x80;
		}
		else
		{
			/* it is not, so set tops with base + ofst,  and clear stat DBF flag */
			VU->vifRegs->tops = VU->vifRegs->base + VU->vifRegs->ofst;
			VU->vifRegs->stat |= 0x80;
		}
	}

	if (VIFdmanum == 0)
		vu0ExecMicro(addr);
	else
		vu1ExecMicro(addr);
}

void vif0Init()
{
	u32 i;

	for (i = 0; i < 256; ++i)
	{
		s_maskwrite[i] = ((i & 3) == 3) || ((i & 0xc) == 0xc) || ((i & 0x30) == 0x30) || ((i & 0xc0) == 0xc0);
	}

	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);
}

static __forceinline void vif0UNPACK(u32 *data)
{
	int vifNum;
	int vl, vn;
	int len;

	if (vif0Regs->cycle.wl == 0 && vif0Regs->cycle.wl < vif0Regs->cycle.cl)
	{
		Console::WriteLn("Vif0 CL %d, WL %d", params vif0Regs->cycle.cl, vif0Regs->cycle.wl);
		vif0.cmd &= ~0x7f;
		return;
	}

	vif0FLUSH();

	vl = (vif0.cmd) & 0x3;
	vn = (vif0.cmd >> 2) & 0x3;
	vif0.tag.addr = (vif0Regs->code & 0x3ff) << 4;
	vif0.usn = (vif0Regs->code >> 14) & 0x1;
	vifNum = (vif0Regs->code >> 16) & 0xff;
	if (vifNum == 0) vifNum = 256;
	vif0Regs->num = vifNum;

	if (vif0Regs->cycle.wl <= vif0Regs->cycle.cl)
	{
		vif0.tag.size = ((vifNum * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
	}
	else
	{
		int n = vif0Regs->cycle.cl * (vifNum / vif0Regs->cycle.wl) +
		        _limit(vifNum % vif0Regs->cycle.wl, vif0Regs->cycle.cl);

		
		vif0.tag.size = ((n * VIFfuncTable[ vif0.cmd & 0xf ].gsize) + 3) >> 2;
	}

	vif0.cl = 0;
	vif0.tag.cmd  = vif0.cmd;
	vif0.tag.addr &= 0xfff;
	vif0Regs->offset = 0;

	
}

static __forceinline void vif0mpgTransfer(u32 addr, u32 *data, int size)
{
	/*	Console::WriteLn("vif0mpgTransfer addr=%x; size=%x", params addr, size);
		{
			FILE *f = fopen("vu1.raw", "wb");
			fwrite(data, 1, size*4, f);
			fclose(f);
		}*/
	if (memcmp(VU0.Micro + addr, data, size << 2))
	{
		CpuVU0.Clear(addr, size << 2); // Clear before writing! :/ (cottonvibes)
		memcpy_fast(VU0.Micro + addr, data, size << 2);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int __fastcall Vif0TransNull(u32 *data)  // Shouldnt go here
{
	Console::WriteLn("VIF0 Shouldnt go here CMD = %x", params vif0Regs->code);
	vif0.cmd = 0;
	return 0;
}
static int __fastcall Vif0TransSTMask(u32 *data)  // STMASK
{
	SetNewMask(g_vif0Masks, g_vif0HasMask3, data[0], vif0Regs->mask);
	vif0Regs->mask = data[0];
	VIF_LOG("STMASK == %x", vif0Regs->mask);

	vif0.tag.size = 0;
	vif0.cmd = 0;
	return 1;
}

static int __fastcall Vif0TransSTRow(u32 *data)  // STROW
{
	int ret;

	u32* pmem = &vif0Regs->r0 + (vif0.tag.addr << 2);
	u32* pmem2 = g_vifRow0 + vif0.tag.addr;
	assert(vif0.tag.addr < 4);
	ret = min(4 - vif0.tag.addr, vif0.vifpacketsize);
	assert(ret > 0);
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;

			jNO_DEFAULT
	}
	vif0.tag.addr += ret;
	vif0.tag.size -= ret;
	if (vif0.tag.size == 0) vif0.cmd = 0;

	return ret;
}

static int __fastcall Vif0TransSTCol(u32 *data)  // STCOL
{
	int ret;

	u32* pmem = &vif0Regs->c0 + (vif0.tag.addr << 2);
	u32* pmem2 = g_vifCol0 + vif0.tag.addr;
	ret = min(4 - vif0.tag.addr, vif0.vifpacketsize);
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;

			jNO_DEFAULT
	}
	vif0.tag.addr += ret;
	vif0.tag.size -= ret;
	if (vif0.tag.size == 0) vif0.cmd = 0;
	return ret;
}

static int __fastcall Vif0TransMPG(u32 *data)  // MPG
{
	if (vif0.vifpacketsize < vif0.tag.size)
	{
		if((vif0.tag.addr + vif0.vifpacketsize) > 0x1000) DevCon::Notice("Vif0 MPG Split Overflow");
		
		vif0mpgTransfer(vif0.tag.addr, data, vif0.vifpacketsize);
		vif0.tag.addr += vif0.vifpacketsize << 2;
		vif0.tag.size -= vif0.vifpacketsize;
		
		return vif0.vifpacketsize;
	}
	else
	{
		int ret;
		
		if((vif0.tag.addr + vif0.tag.size) > 0x1000) DevCon::Notice("Vif0 MPG Overflow");
		
		vif0mpgTransfer(vif0.tag.addr, data, vif0.tag.size);
		ret = vif0.tag.size;
		vif0.tag.size = 0;
		vif0.cmd = 0;
		
		return ret;
	}
}

static int __fastcall Vif0TransUnpack(u32 *data)	// UNPACK
{
	int ret;
	
	FreezeXMMRegs(1);
	if (vif0.vifpacketsize < vif0.tag.size)
	{
		if(vif0Regs->offset != 0 || vif0.cl != 0) 
		{
			ret = vif0.tag.size;
			vif0.tag.size -= vif0.vifpacketsize - VIFalign(data, &vif0.tag, vif0.vifpacketsize, VIF0dmanum);
			ret = ret - vif0.tag.size;
			data += ret;
			
			if(vif0.vifpacketsize > 0) VIFunpack(data, &vif0.tag, vif0.vifpacketsize - ret, VIF0dmanum);
			
			ProcessMemSkip((vif0.vifpacketsize - ret) << 2, (vif0.cmd & 0xf), VIF0dmanum);
			vif0.tag.size -= (vif0.vifpacketsize - ret);
			FreezeXMMRegs(0);
			
			return vif0.vifpacketsize;
		} 
		/* size is less that the total size, transfer is 'in pieces' */
		VIFunpack(data, &vif0.tag, vif0.vifpacketsize, VIF0dmanum);
		
		ProcessMemSkip(vif0.vifpacketsize << 2, (vif0.cmd & 0xf), VIF0dmanum);

		ret = vif0.vifpacketsize;
		vif0.tag.size -= ret;
	}
	else
	{
		/* we got all the data, transfer it fully */
		ret = vif0.tag.size;
		
		//Align data after a split transfer first
		if ((vif0Regs->offset != 0) || (vif0.cl != 0)) 
		{
			vif0.tag.size = VIFalign(data, &vif0.tag, vif0.tag.size, VIF0dmanum);
			data += ret - vif0.tag.size;
			if(vif0.tag.size > 0) VIFunpack(data, &vif0.tag, vif0.tag.size, VIF0dmanum);
		}
		else
		{
			VIFunpack(data, &vif0.tag, vif0.tag.size, VIF0dmanum);
		}
		
		vif0.tag.size = 0;
		vif0.cmd = 0;
	}
	
	FreezeXMMRegs(0);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif0 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Vif0CMDNop()  // NOP
{
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTCycl()  // STCYCL
{
	vif0Regs->cycle.cl = (u8)vif0Regs->code;
	vif0Regs->cycle.wl = (u8)(vif0Regs->code >> 8);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDITop()  // ITOP
{
	vif0Regs->itops = vif0Regs->code & 0x3ff;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMod()  // STMOD
{
	vif0Regs->mode = vif0Regs->code & 0x3;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMark()  // MARK
{
	vif0Regs->mark = (u16)vif0Regs->code;
	vif0Regs->stat |= VIF0_STAT_MRK;
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDFlushE()  // FLUSHE
{
	vif0FLUSH();
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCALF()  //MSCAL/F
{
	vuExecMicro((u16)(vif0Regs->code) << 3, VIF0dmanum);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDMSCNT()  // MSCNT
{
	vuExecMicro(-1, VIF0dmanum);
	vif0.cmd &= ~0x7f;
}

static void Vif0CMDSTMask()  // STMASK
{
	vif0.tag.size = 1;
}

static void Vif0CMDSTRowCol() // STROW / STCOL
{
	vif0.tag.addr = 0;
	vif0.tag.size = 4;
}

static void Vif0CMDMPGTransfer()  // MPG
{
	int vifNum;
	vif0FLUSH();
	vifNum = (u8)(vif0Regs->code >> 16);
	if (vifNum == 0) vifNum = 256;
	vif0.tag.addr = (u16)((vif0Regs->code) << 3) & 0xfff;
	vif0.tag.size = vifNum * 2;
}

static void Vif0CMDNull()  // invalid opcode
{
	// if ME1, then force the vif to interrupt
	if ((vif0Regs->err & 0x4) == 0)    //Ignore vifcode and tag mismatch error
	{
		Console::WriteLn("UNKNOWN VifCmd: %x", params vif0.cmd);
		vif0Regs->stat |= 1 << 13;
		vif0.irq++;
	}
	vif0.cmd &= ~0x7f;
}

int VIF0transfer(u32 *data, int size, int istag)
{
	int ret;
	int transferred = vif0.vifstalled ? vif0.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)
	VIF_LOG("VIF0transfer: size %x (vif0.cmd %x)", size, vif0.cmd);

	vif0.stallontag = false;
	vif0.vifstalled = false;
	vif0.vifpacketsize = size;

	while (vif0.vifpacketsize > 0)
	{
		if (vif0.cmd)
		{
			vif0Regs->stat |= VIF0_STAT_VPS_T; //Decompression has started
			
			ret = Vif0TransTLB[(vif0.cmd & 0x7f)](data);
			data += ret;
			vif0.vifpacketsize -= ret;
			if (vif0.cmd == 0) vif0Regs->stat &= ~VIF0_STAT_VPS_T; //We are once again waiting for a new vifcode as the command has cleared
			continue;
		}

		if (vif0.tag.size != 0) Console::WriteLn("no vif0 cmd but tag size is left last cmd read %x", params vif0Regs->code);
		
		// if interrupt and new cmd is NOT MARK
		if (vif0.irq) break;

		vif0.cmd = (data[0] >> 24);
		vif0Regs->code = data[0];

		vif0Regs->stat |= VIF0_STAT_VPS_D; //We need to set these (Onimusha needs it)

		if ((vif0.cmd & 0x60) == 0x60)
		{
			vif0UNPACK(data);
		}
		else
		{
			VIF_LOG("VIFtransfer: cmd %x, num %x, imm %x, size %x", vif0.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, size);

			if ((vif0.cmd & 0x7f) > 0x4A)
			{
				if ((vif0Regs->err & 0x4) == 0)    //Ignore vifcode and tag mismatch error
				{
					Console::WriteLn("UNKNOWN VifCmd: %x", params vif0.cmd);
					vif0Regs->stat |= 1 << 13;
					vif0.irq++;
				}
				vif0.cmd = 0;
			}
			else 
			{
				Vif0CMDTLB[(vif0.cmd & 0x7f)]();
			}
		}
		++data;
		--vif0.vifpacketsize;

		if ((vif0.cmd & 0x80))
		{
			vif0.cmd &= 0x7f;

			if (!(vif0Regs->err & 0x1)) //i bit on vifcode and not masked by VIF0_ERR
			{
				VIF_LOG("Interrupt on VIFcmd: %x (INTC_MASK = %x)", vif0.cmd, psHu32(INTC_MASK));

				++vif0.irq;

				if (istag && vif0.tag.size <= vif0.vifpacketsize) vif0.stallontag = true;

				if (vif0.tag.size == 0) break;
			}
		}

	} //End of Transfer loop

	transferred += size - vif0.vifpacketsize;
	g_vifCycles += (transferred >> 2) * BIAS; /* guessing */
	// use tag.size because some game doesn't like .cmd

	if (vif0.irq && (vif0.tag.size == 0))
	{
		vif0.vifstalled = true;

		if (((vif0Regs->code >> 24) & 0x7f) != 0x7)vif0Regs->stat |= VIF0_STAT_VIS;
		//else Console::WriteLn("VIF0 IRQ on MARK");
		
		// spiderman doesn't break on qw boundaries
		vif0.irqoffset = transferred % 4; // cannot lose the offset

		if (!istag) 
		{
			transferred = transferred >> 2;
			vif0ch->madr += (transferred << 4);
			vif0ch->qwc -= transferred;
		}
		//else Console::WriteLn("Stall on vif0, FromSPR = %x, Vif0MADR = %x Sif0MADR = %x STADR = %x", params psHu32(0x1000d010), vif0ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return -2;
	}

	vif0Regs->stat &= ~VIF0_STAT_VPS; //Vif goes idle as the stall happened between commands;
	if (vif0.cmd) vif0Regs->stat |= VIF0_STAT_VPS_W;  //Otherwise we wait for the data

	if (!istag)
	{
		transferred = transferred >> 2;
		vif0ch->madr += (transferred << 4);
		vif0ch->qwc -= transferred;
	}

	return 0;
}

int  _VIF0chain()
{
	u32 *pMem;
	u32 ret;

	if ((vif0ch->qwc == 0) && !vif0.vifstalled) return 0;

	pMem = (u32*)dmaGetAddr(vif0ch->madr);
	if (pMem == NULL)
	{
		vif0.cmd = 0;
		vif0.tag.size = 0;
		vif0ch->qwc = 0;
		return 0;
	}

	if (vif0.vifstalled)
		ret = VIF0transfer(pMem + vif0.irqoffset, vif0ch->qwc * 4 - vif0.irqoffset, 0);
	else
		ret = VIF0transfer(pMem, vif0ch->qwc * 4, 0);

	return ret;
}

int _chainVIF0()
{
	int id, ret;

	vif0ptag = (u32*)dmaGetAddr(vif0ch->tadr); //Set memory pointer to TADR
	if (vif0ptag == NULL)  						//Is vif0ptag empty?
	{
		Console::Error("Vif0 Tag BUSERR");
		vif0ch->chcr = (vif0ch->chcr & 0xFFFF) | ((*vif0ptag) & 0xFFFF0000);     //Transfer upper part of tag to CHCR bits 31-15
		psHu32(DMAC_STAT) |= 1 << 15;       //If yes, set BEIS (BUSERR) in DMAC_STAT register
		return -1;						   //Return -1 as an error has occurred
	}

	id = (vif0ptag[0] >> 28) & 0x7; //ID for DmaChain copied from bit 28 of the tag
	vif0ch->qwc  = (u16)vif0ptag[0];       //QWC set to lower 16bits of the tag
	vif0ch->madr = vif0ptag[1];            //MADR = ADDR field
	g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag
	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        vif0ptag[1], vif0ptag[0], vif0ch->qwc, id, vif0ch->madr, vif0ch->tadr);

	vif0ch->chcr = (vif0ch->chcr & 0xFFFF) | ((*vif0ptag) & 0xFFFF0000);     //Transfer upper part of tag to CHCR bits 31-15
	// Transfer dma tag if tte is set

	if (vif0ch->chcr & 0x40)
	{
		if (vif0.vifstalled) 
			ret = VIF0transfer(vif0ptag + (2 + vif0.irqoffset), 2 - vif0.irqoffset, 1);  //Transfer Tag on stall
		else 
			ret = VIF0transfer(vif0ptag + 2, 2, 1);  //Transfer Tag
		if (ret == -1) return -1;       //There has been an error
		if (ret == -2) return -2;        //IRQ set by VIFTransfer
	}

	vif0.done |= hwDmacSrcChainWithStack(vif0ch, id);

	VIF_LOG("dmaChain %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx",
	        vif0ptag[1], vif0ptag[0], vif0ch->qwc, id, vif0ch->madr, vif0ch->tadr);

	ret = _VIF0chain();											   //Transfers the data set by the switch

	if ((vif0ch->chcr & 0x80) && (vif0ptag[0] >> 31))  			       //Check TIE bit of CHCR and IRQ bit of tag
	{
		VIF_LOG("dmaIrq Set\n");

		vif0.done = true;												   //End Transfer
	}
	return (vif0.done) ? 1: 0;												   //Return Done
}

void  vif0Interrupt()
{
	g_vifCycles = 0; //Reset the cycle count, Wouldn't reset on stall if put lower down.
	VIF_LOG("vif0Interrupt: %8.8x", cpuRegs.cycle);

	if (vif0.irq && (vif0.tag.size == 0))
	{
		vif0Regs->stat |= VIF0_STAT_INT;
		hwIntcIrq(VIF0intc);
		--vif0.irq;

		if (vif0Regs->stat & (VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
		{
			vif0Regs->stat &= ~0xF000000; // FQC=0
			vif0ch->chcr &= ~0x100;
			return;
		}
		
		if (vif0ch->qwc > 0 || vif0.irqoffset > 0)
		{
			if (vif0.stallontag)
				_chainVIF0();
			else 
				_VIF0chain();
			
			CPU_INT(0, g_vifCycles);
			return;
		}
	}

	if ((vif0ch->chcr & 0x100) == 0) Console::WriteLn("Vif0 running when CHCR = %x", params vif0ch->chcr);

	if ((vif0ch->chcr & 0x4) && (!vif0.done) && (!vif0.vifstalled))
	{

		if (!(psHu32(DMAC_CTRL) & 0x1))
		{
			Console::WriteLn("vif0 dma masked");
			return;
		}

		if (vif0ch->qwc > 0)
			_VIF0chain();
		else 
			_chainVIF0();
		
		CPU_INT(0, g_vifCycles);
		return;
	}

	if (vif0ch->qwc > 0) Console::WriteLn("VIF0 Ending with QWC left");
	if (vif0.cmd != 0) Console::WriteLn("vif0.cmd still set %x", params vif0.cmd);
	
	vif0ch->chcr &= ~0x100;
	hwDmacIrq(DMAC_VIF0);
	vif0Regs->stat &= ~0xF000000; // FQC=0
}

//  Vif1 Data Transfer Table
int (__fastcall *Vif0TransTLB[128])(u32 *data) =
{
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x7*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0xF*/
	Vif0TransNull	 , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x17*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x1F*/
	Vif0TransSTMask  , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x27*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x2F*/
	Vif0TransSTRow	 , Vif0TransSTCol	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull   , /*0x37*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x3F*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x47*/
	Vif0TransNull    , Vif0TransNull    , Vif0TransMPG	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x4F*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull	  , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , /*0x57*/
	Vif0TransNull	 , Vif0TransNull	, Vif0TransNull	  , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , Vif0TransNull   , /*0x5F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x67*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , /*0x6F*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransNull   , /*0x77*/
	Vif0TransUnpack  , Vif0TransUnpack  , Vif0TransUnpack , Vif0TransNull   , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack , Vif0TransUnpack   /*0x7F*/
};

// Vif1 CMD Table
void (*Vif0CMDTLB[75])() =
{
	Vif0CMDNop	   , Vif0CMDSTCycl  , Vif0CMDNull		, Vif0CMDNull , Vif0CMDITop  , Vif0CMDSTMod , Vif0CMDNull, Vif0CMDMark , /*0x7*/
	Vif0CMDNull	   , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0xF*/
	Vif0CMDFlushE   , Vif0CMDNull   , Vif0CMDNull		, Vif0CMDNull, Vif0CMDMSCALF, Vif0CMDMSCALF, Vif0CMDNull	, Vif0CMDMSCNT, /*0x17*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x1F*/
	Vif0CMDSTMask  , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x27*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x2F*/
	Vif0CMDSTRowCol, Vif0CMDSTRowCol, Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull	, Vif0CMDNull , /*0x37*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x3F*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDNull		, Vif0CMDNull , Vif0CMDNull  , Vif0CMDNull  , Vif0CMDNull    , Vif0CMDNull , /*0x47*/
	Vif0CMDNull    , Vif0CMDNull    , Vif0CMDMPGTransfer
};

void dmaVIF0()
{
	VIF_LOG("dmaVIF0 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx\n",
	        vif0ch->chcr, vif0ch->madr, vif0ch->qwc,
	        vif0ch->tadr, vif0ch->asr0, vif0ch->asr1);

	g_vifCycles = 0;

	vif0Regs->stat |= 0x8000000; // FQC=8

	if (!(vif0ch->chcr & 0x4) || vif0ch->qwc > 0)   // Normal Mode
	{
		if (_VIF0chain() == -2)
		{
			Console::WriteLn("Stall on normal %x", params vif0Regs->stat);
			vif0.vifstalled = true;
			return;
		}
		
		vif0.done = true;
		CPU_INT(0, g_vifCycles);
		return;
	}

	// Chain Mode
	vif0.done = false;
	CPU_INT(0, 0);
}

void vif0Write32(u32 mem, u32 value)
{
	switch (mem)
	{
		case VIF0_MARK:
			VIF_LOG("VIF0_MARK write32 0x%8.8x", value);

			/* Clear mark flag in VIF0_STAT and set mark with 'value' */
			vif0Regs->stat &= ~VIF0_STAT_MRK;
			vif0Regs->mark = value;
			break;
		
		case VIF0_FBRST:
			VIF_LOG("VIF0_FBRST write32 0x%8.8x", value);

			if (value & 0x1)
			{
				/* Reset VIF */
				//Console::WriteLn("Vif0 Reset %x", params vif0Regs->stat);
				memzero_obj(vif0);
				vif0ch->qwc = 0; //?
				cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
				psHu64(VIF0_FIFO) = 0;
				psHu64(0x10004008) = 0; // VIF0_FIFO + 8
				vif0.done = true;
				vif0Regs->err = 0;
				vif0Regs->stat &= ~(0xF000000 | VIF0_STAT_INT | VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS | VIF0_STAT_VPS); // FQC=0
			}
			
			if (value & 0x2)
			{
				/* Force Break the VIF */
				/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
				cpuRegs.interrupt &= ~1; //Stop all vif0 DMA's
				vif0Regs->stat |= VIF0_STAT_VFS;
				vif0Regs->stat &= ~VIF0_STAT_VPS;
				vif0.vifstalled = true;
				Console::WriteLn("vif0 force break");
			}
				
			if (value & 0x4)
			{
				/* Stop VIF */
				// Not completely sure about this, can't remember what game, used this, but 'draining' the VIF helped it, instead of
				//  just stoppin the VIF (linuz).
				vif0Regs->stat |= VIF0_STAT_VSS;
				vif0Regs->stat &= ~VIF0_STAT_VPS;
				vif0.vifstalled = true;
			}
			
			if (value & 0x8)
			{
				bool cancel = false;

				/* Cancel stall, first check if there is a stall to cancel, and then clear VIF0_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
				if (vif0Regs->stat & (VIF0_STAT_VSS | VIF0_STAT_VIS | VIF0_STAT_VFS))
					cancel = true;

				vif0Regs->stat &= ~(VIF0_STAT_VSS | VIF0_STAT_VFS | VIF0_STAT_VIS |
						    VIF0_STAT_INT | VIF0_STAT_ER0 | VIF0_STAT_ER1);
				if (cancel)
				{
					if (vif0.vifstalled)
					{
						g_vifCycles = 0;

						// loop necessary for spiderman
						if (vif0.stallontag)
							_chainVIF0();
						else
							_VIF0chain();

						vif0ch->chcr |= 0x100;
						CPU_INT(0, g_vifCycles); // Gets the timing right - Flatout
					}
				}
			}
			break;
			
		case VIF0_ERR:
			// ERR
			VIF_LOG("VIF0_ERR write32 0x%8.8x", value);

			/* Set VIF0_ERR with 'value' */
			vif0Regs->err = value;
			break;
		
		case VIF0_R0:
		case VIF0_R1:
		case VIF0_R2:
		case VIF0_R3:
			assert((mem&0xf) == 0);
			g_vifRow0[(mem>>4) & 3] = value;
			break;
		
		case VIF0_C0:
		case VIF0_C1:
		case VIF0_C2:
		case VIF0_C3:
			assert((mem&0xf) == 0);
			g_vifCol0[(mem>>4) & 3] = value;
			break;
		
		default:
			Console::WriteLn("Unknown Vif0 write to %x", params mem);
			psHu32(mem) = value;
			break;
	}
	/* Other registers are read-only so do nothing for them */
}

void vif0Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars and all the registers */
	memzero_obj(vif0);
	memzero_obj(*vif0Regs);
	SetNewMask(g_vif0Masks, g_vif0HasMask3, 0, 0xffffffff);
	psHu64(VIF0_FIFO) = 0;
	psHu64(0x10004008) = 0; // VIF0_FIFO + 8
	vif0Regs->stat &= ~VIF0_STAT_VPS;
	vif0.done = true;
	vif0Regs->stat &= ~0xF000000; // FQC=0
}

void SaveState::vif0Freeze()
{
	FreezeTag("VIFdma");

	// Dunno if this one is needed, but whatever, it's small. :)
	Freeze(g_vifCycles);

	Freeze(vif0);

	Freeze(g_vif0HasMask3);
	Freeze(g_vif0Masks);
	Freeze(g_vifRow0);
	Freeze(g_vifCol0);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


void vif1Init()
{
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
}

static __forceinline void vif1UNPACK(u32 *data)
{
	int vifNum;
	int vl, vn;

	if (vif1Regs->cycle.wl == 0)
	{
		if (vif1Regs->cycle.wl < vif1Regs->cycle.cl)
		{
			Console::WriteLn("Vif1 CL %d, WL %d", params vif1Regs->cycle.cl, vif1Regs->cycle.wl);
			vif1.cmd &= ~0x7f;
			return;
		}
	}
	//vif1FLUSH();

	vl = (vif1.cmd) & 0x3;
	vn = (vif1.cmd >> 2) & 0x3;

	vif1.usn = (vif1Regs->code >> 14) & 0x1;
	vifNum = (vif1Regs->code >> 16) & 0xff;

	if (vifNum == 0) vifNum = 256;
	vif1Regs->num = vifNum;

	if (vif1Regs->cycle.wl <= vif1Regs->cycle.cl)
	{
		vif1.tag.size = ((vifNum * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
	}
	else
	{
		int n = vif1Regs->cycle.cl * (vifNum / vif1Regs->cycle.wl) +
		        _limit(vifNum % vif1Regs->cycle.wl, vif1Regs->cycle.cl);
		vif1.tag.size = ((n * VIFfuncTable[ vif1.cmd & 0xf ].gsize) + 3) >> 2;
		
	}
	
	
	if ((vif1Regs->code >> 15) & 0x1)
		vif1.tag.addr = (vif1Regs->code + vif1Regs->tops) & 0x3ff;
	else
		vif1.tag.addr = vif1Regs->code & 0x3ff;

	vif1Regs->offset = 0;
	vif1.cl = 0;
	vif1.tag.addr <<= 4;
	vif1.tag.cmd  = vif1.cmd;

}

static __forceinline void vif1mpgTransfer(u32 addr, u32 *data, int size)
{
	/*	Console::WriteLn("vif1mpgTransfer addr=%x; size=%x", params addr, size);
		{
			FILE *f = fopen("vu1.raw", "wb");
			fwrite(data, 1, size*4, f);
			fclose(f);
		}*/
	assert(VU1.Micro > 0);
	if (memcmp(VU1.Micro + addr, data, size << 2))
	{
		CpuVU1.Clear(addr, size << 2); // Clear before writing! :/
		memcpy_fast(VU1.Micro + addr, data, size << 2);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 Data Transfer Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int __fastcall Vif1TransNull(u32 *data)  // Shouldnt go here
{
	Console::WriteLn("Shouldnt go here CMD = %x", params vif1Regs->code);
	vif1.cmd = 0;
	return 0;
}
static int __fastcall Vif1TransSTMask(u32 *data)  // STMASK
{
	SetNewMask(g_vif1Masks, g_vif1HasMask3, data[0], vif1Regs->mask);
	vif1Regs->mask = data[0];
	VIF_LOG("STMASK == %x", vif1Regs->mask);

	vif1.tag.size = 0;
	vif1.cmd = 0;
	return 1;
}

static int __fastcall Vif1TransSTRow(u32 *data)
{
	int ret;

	u32* pmem = &vif1Regs->r0 + (vif1.tag.addr << 2);
	u32* pmem2 = g_vifRow1 + vif1.tag.addr;
	assert(vif1.tag.addr < 4);
	ret = min(4 - vif1.tag.addr, vif1.vifpacketsize);
	assert(ret > 0);
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;
		jNO_DEFAULT;
	}
	vif1.tag.addr += ret;
	vif1.tag.size -= ret;
	if (vif1.tag.size == 0) vif1.cmd = 0;

	return ret;
}

static int __fastcall Vif1TransSTCol(u32 *data)
{
	int ret;

	u32* pmem = &vif1Regs->c0 + (vif1.tag.addr << 2);
	u32* pmem2 = g_vifCol1 + vif1.tag.addr;
	ret = min(4 - vif1.tag.addr, vif1.vifpacketsize);
	switch (ret)
	{
		case 4:
			pmem[12] = data[3];
			pmem2[3] = data[3];
		case 3:
			pmem[8] = data[2];
			pmem2[2] = data[2];
		case 2:
			pmem[4] = data[1];
			pmem2[1] = data[1];
		case 1:
			pmem[0] = data[0];
			pmem2[0] = data[0];
			break;
			jNO_DEFAULT;
	}
	vif1.tag.addr += ret;
	vif1.tag.size -= ret;
	if (vif1.tag.size == 0) vif1.cmd = 0;
	return ret;
}

static int __fastcall Vif1TransMPG(u32 *data)
{
	if (vif1.vifpacketsize < vif1.tag.size)
	{
		if((vif1.tag.addr + vif1.vifpacketsize) > 0x4000) DevCon::Notice("Vif1 MPG Split Overflow");
		vif1mpgTransfer(vif1.tag.addr, data, vif1.vifpacketsize);
		vif1.tag.addr += vif1.vifpacketsize << 2;
		vif1.tag.size -= vif1.vifpacketsize;
		return vif1.vifpacketsize;
	}
	else
	{
		int ret;
		if((vif1.tag.addr + vif1.tag.size) > 0x4000) DevCon::Notice("Vif1 MPG Overflow");
		vif1mpgTransfer(vif1.tag.addr, data, vif1.tag.size);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
		return ret;
	}
}

static int __fastcall Vif1TransDirectHL(u32 *data)
{
	int ret = 0;

	if((vif1.cmd & 0x7f) == 0x51)
	{
		if(gif->chcr & 0x100 && (!vif1Regs->mskpath3 && Path3progress == 0)) //PATH3 is in image mode, so wait for end of transfer
		{
			vif1Regs->stat |= VIF1_STAT_VGW;
			return 0;
		}			
	}

	psHu32(GIF_STAT) |= (GIF_STAT_APATH2 | GIF_STAT_OPH);

	if (splitptr > 0)  //Leftover data from the last packet, filling the rest and sending to the GS
	{
		if ((splitptr < 4) && (vif1.vifpacketsize >= (4 - splitptr)))
		{
			while (splitptr < 4)
			{
				splittransfer[splitptr++] = (u32)data++;
				ret++;
				vif1.tag.size--;
			}
		}

		if (mtgsThread != NULL)
		{
			// copy 16 bytes the fast way:
			const u64* src = (u64*)splittransfer[0];
			const uint count = mtgsThread->PrepDataPacket(GIF_PATH_2, src, 1);
			jASSUME(count == 1);
			u64* dst = (u64*)mtgsThread->GetDataPacketPtr();
			dst[0] = src[0];
			dst[1] = src[1];

			mtgsThread->SendDataPacket();
		}
		else
		{
			FreezeRegs(1);
			GSGIFTRANSFER2((u32*)splittransfer[0], 1);
			FreezeRegs(0);
		}

		if (vif1.tag.size == 0) vif1.cmd = 0;
		splitptr = 0;
		return ret;
	}
	if (vif1.vifpacketsize < vif1.tag.size)
	{
		if (vif1.vifpacketsize < 4 && splitptr != 4)   //Not a full QW left in the buffer, saving left over data
		{
			ret = vif1.vifpacketsize;
			while (ret > 0)
			{
				splittransfer[splitptr++] = (u32)data++;
				vif1.tag.size--;
				ret--;
			}
			return vif1.vifpacketsize;
		}
		vif1.tag.size -= vif1.vifpacketsize;
		ret = vif1.vifpacketsize;
	}
	else
	{
		psHu32(GIF_STAT) &= ~(GIF_STAT_APATH2 | GIF_STAT_OPH);
		ret = vif1.tag.size;
		vif1.tag.size = 0;
		vif1.cmd = 0;
	}

	//TODO: ret is guaranteed to be qword aligned ?

	if (mtgsThread != NULL)
	{
		//unaligned copy.VIF handling is -very- messy, so i'l use this code til i fix it :)
		// Round ret up, just in case it's not 128bit aligned.
		const uint count = mtgsThread->PrepDataPacket(GIF_PATH_2, data, (ret + 3) >> 2);
		memcpy_fast(mtgsThread->GetDataPacketPtr(), data, count << 4);
		mtgsThread->SendDataPacket();
	}
	else
	{
		FreezeRegs(1);
		GSGIFTRANSFER2(data, (ret >> 2));
		FreezeRegs(0);
	}

	return ret;
}

static int  __fastcall Vif1TransUnpack(u32 *data)
{
	FreezeXMMRegs(1);

	if (vif1.vifpacketsize < vif1.tag.size)
	{
		int ret = vif1.tag.size;
		/* size is less that the total size, transfer is
		   'in pieces' */
		if(vif1Regs->offset != 0 || vif1.cl != 0) 
		{
			vif1.tag.size -= vif1.vifpacketsize - VIFalign(data, &vif1.tag, vif1.vifpacketsize, VIF1dmanum);
			ret = ret - vif1.tag.size;
			data += ret;
			if((vif1.vifpacketsize - ret) > 0) VIFunpack(data, &vif1.tag, vif1.vifpacketsize - ret, VIF1dmanum);
			ProcessMemSkip((vif1.vifpacketsize - ret) << 2, (vif1.cmd & 0xf), VIF1dmanum);
			vif1.tag.size -= (vif1.vifpacketsize - ret);
			FreezeXMMRegs(0);
			return vif1.vifpacketsize;
		} 
		VIFunpack(data, &vif1.tag, vif1.vifpacketsize, VIF1dmanum);

		ProcessMemSkip(vif1.vifpacketsize << 2, (vif1.cmd & 0xf), VIF1dmanum);
		vif1.tag.size -= vif1.vifpacketsize;
		FreezeXMMRegs(0);
		return vif1.vifpacketsize;
	}
	else
	{
		int ret = vif1.tag.size;

		if(vif1Regs->offset != 0 || vif1.cl != 0) 
		{
			vif1.tag.size = VIFalign(data, &vif1.tag, vif1.tag.size, VIF1dmanum);
			data += ret - vif1.tag.size;
			if(vif1.tag.size > 0) VIFunpack(data, &vif1.tag, vif1.tag.size, VIF1dmanum);
			vif1.tag.size = 0;
			vif1.cmd = 0;
			FreezeXMMRegs(0);
			return ret;
		} 
		else
		{
			/* we got all the data, transfer it fully */
			VIFunpack(data, &vif1.tag, vif1.tag.size, VIF1dmanum);
			vif1.tag.size = 0;
			vif1.cmd = 0;
			FreezeXMMRegs(0);
			return ret;
		}
	}

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Base Commands
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void Vif1CMDNop()  // NOP
{
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTCycl()  // STCYCL
{
	vif1Regs->cycle.cl = (u8)vif1Regs->code;
	vif1Regs->cycle.wl = (u8)(vif1Regs->code >> 8);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDOffset()  // OFFSET
{
	vif1Regs->ofst  = vif1Regs->code & 0x3ff;
	vif1Regs->stat &= ~0x80;
	vif1Regs->tops  = vif1Regs->base;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDBase()  // BASE
{
	vif1Regs->base = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDITop()  // ITOP
{
	vif1Regs->itops = vif1Regs->code & 0x3ff;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTMod()  // STMOD
{
	vif1Regs->mode = vif1Regs->code & 0x3;
	vif1.cmd &= ~0x7f;
}

u8 schedulepath3msk = 0;

void Vif1MskPath3()  // MSKPATH3
{
	vif1Regs->mskpath3 = schedulepath3msk & 0x1;
	//Console::WriteLn("VIF MSKPATH3 %x", params vif1Regs->mskpath3);

	if (vif1Regs->mskpath3)
	{
		psHu32(GIF_STAT) |= 0x2;
	}
	else
	{
		Path3progress = 1; //Let the Gif know it can transfer again (making sure any vif stall isnt unset prematurely)
		psHu32(GIF_STAT) &= ~0x2;
		CPU_INT(2, 4);		
	}
	
	schedulepath3msk = 0;
}
static void Vif1CMDMskPath3()  // MSKPATH3
{
	if(vif1ch->chcr & 0x100)
	{
		schedulepath3msk = 0x10 | ((vif1Regs->code >> 15) & 0x1);
		vif1.vifstalled = true;
	}
	else 
	{
		schedulepath3msk = (vif1Regs->code >> 15) & 0x1;
		Vif1MskPath3();
	}
	vif1.cmd &= ~0x7f;
}


static void Vif1CMDMark()  // MARK
{
	vif1Regs->mark = (u16)vif1Regs->code;
	vif1Regs->stat |= VIF1_STAT_MRK;
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDFlush()  // FLUSH/E/A
{
	vif1FLUSH();

	if((vif1.cmd & 0x7f) == 0x13)
	{
		if((Path3progress != 2 || !vif1Regs->mskpath3) && gif->chcr & 0x100) // Gif is already transferring so wait for it.
		{	
			vif1Regs->stat |= VIF1_STAT_VGW;	
			CPU_INT(2, 4);
		}
	}

	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMSCALF()  //MSCAL/F
{
	vif1FLUSH();
	vuExecMicro((u16)(vif1Regs->code) << 3, VIF1dmanum);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDMSCNT()  // MSCNT
{
	vuExecMicro(-1, VIF1dmanum);
	vif1.cmd &= ~0x7f;
}

static void Vif1CMDSTMask()  // STMASK
{
	vif1.tag.size = 1;
}

static void Vif1CMDSTRowCol() // STROW / STCOL
{
	vif1.tag.addr = 0;
	vif1.tag.size = 4;
}

static void Vif1CMDMPGTransfer()  // MPG
{
	int vifNum;
	//vif1FLUSH();
	vifNum = (u8)(vif1Regs->code >> 16);

	if (vifNum == 0) vifNum = 256;

	vif1.tag.addr = (u16)((vif1Regs->code) << 3) & 0x3fff;
	vif1.tag.size = vifNum * 2;
}

static void Vif1CMDDirectHL()  // DIRECT/HL
{
	int vifImm;
	vifImm = (u16)vif1Regs->code;

	if (vifImm == 0)
		vif1.tag.size = 65536 << 2;
	else
		vif1.tag.size = vifImm << 2;
}

static void Vif1CMDNull()  // invalid opcode
{
	// if ME1, then force the vif to interrupt

	if ((vif1Regs->err & 0x4) == 0)   //Ignore vifcode and tag mismatch error
	{
		Console::WriteLn("UNKNOWN VifCmd: %x\n", params vif1.cmd);
		vif1Regs->stat |= 1 << 13;
		vif1.irq++;
	}
	vif1.cmd = 0;
}

//  Vif1 Data Transfer Table

int (__fastcall *Vif1TransTLB[128])(u32 *data) =
{
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x7*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0xF*/
	Vif1TransNull	 , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x17*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x1F*/
	Vif1TransSTMask  , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x27*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x2F*/
	Vif1TransSTRow	 , Vif1TransSTCol	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull   , /*0x37*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x3F*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x47*/
	Vif1TransNull    , Vif1TransNull    , Vif1TransMPG	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x4F*/
	Vif1TransDirectHL, Vif1TransDirectHL, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull	  , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , /*0x57*/
	Vif1TransNull	 , Vif1TransNull	, Vif1TransNull	  , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , Vif1TransNull   , /*0x5F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x67*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , /*0x6F*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransNull   , /*0x77*/
	Vif1TransUnpack  , Vif1TransUnpack  , Vif1TransUnpack , Vif1TransNull   , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack , Vif1TransUnpack   /*0x7F*/
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vif1 CMD Table
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void (*Vif1CMDTLB[82])() =
{
	Vif1CMDNop	   , Vif1CMDSTCycl  , Vif1CMDOffset		, Vif1CMDBase , Vif1CMDITop  , Vif1CMDSTMod , Vif1CMDMskPath3, Vif1CMDMark , /*0x7*/
	Vif1CMDNull	   , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0xF*/
	Vif1CMDFlush   , Vif1CMDFlush   , Vif1CMDNull		, Vif1CMDFlush, Vif1CMDMSCALF, Vif1CMDMSCALF, Vif1CMDNull	, Vif1CMDMSCNT, /*0x17*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x1F*/
	Vif1CMDSTMask  , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x27*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x2F*/
	Vif1CMDSTRowCol, Vif1CMDSTRowCol, Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull	, Vif1CMDNull , /*0x37*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x3F*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDNull		, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x47*/
	Vif1CMDNull    , Vif1CMDNull    , Vif1CMDMPGTransfer, Vif1CMDNull , Vif1CMDNull  , Vif1CMDNull  , Vif1CMDNull    , Vif1CMDNull , /*0x4F*/
	Vif1CMDDirectHL, Vif1CMDDirectHL
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VIF1transfer(u32 *data, int size, int istag)
{
	int ret;
	int transferred = vif1.vifstalled ? vif1.irqoffset : 0; // irqoffset necessary to add up the right qws, or else will spin (spiderman)

	VIF_LOG("VIF1transfer: size %x (vif1.cmd %x)", size, vif1.cmd);

	vif1.irqoffset = 0;
	vif1.vifstalled = false;
	vif1.stallontag = false;
	vif1.vifpacketsize = size;

	while (vif1.vifpacketsize > 0)
	{		
		if(vif1Regs->stat & VIF1_STAT_VGW) break;
		
		if (vif1.cmd)
		{
			vif1Regs->stat |= VIF1_STAT_VPS_T; //Decompression has started
			
			ret = Vif1TransTLB[vif1.cmd](data);
			data += ret;
			vif1.vifpacketsize -= ret;
			if (vif1.cmd == 0) vif1Regs->stat &= ~VIF1_STAT_VPS_T; //We are once again waiting for a new vifcode as the command has cleared
			continue;
		}

		if (vif1.tag.size != 0) DevCon::Error("no vif1 cmd but tag size is left last cmd read %x", params vif1Regs->code);

		if (vif1.irq) break;

		vif1.cmd = (data[0] >> 24);
		vif1Regs->code = data[0];

		vif1Regs->stat |= VIF1_STAT_VPS_D;
		if ((vif1.cmd & 0x60) == 0x60)
		{
			vif1UNPACK(data);
		}
		else
		{
			VIF_LOG("VIFtransfer: cmd %x, num %x, imm %x, size %x", vif1.cmd, (data[0] >> 16) & 0xff, data[0] & 0xffff, vif1.vifpacketsize);

			if ((vif1.cmd & 0x7f) > 0x51)
			{
				if ((vif1Regs->err & 0x4) == 0)    //Ignore vifcode and tag mismatch error
				{
					Console::WriteLn("UNKNOWN VifCmd: %x", params vif1.cmd);
					vif1Regs->stat |= 1 << 13;
					vif1.irq++;
				}
				vif1.cmd = 0;
			}
			else Vif1CMDTLB[(vif1.cmd & 0x7f)]();
		}

		++data;
		--vif1.vifpacketsize;

		if ((vif1.cmd & 0x80))
		{
			vif1.cmd &= 0x7f;

			if (!(vif1Regs->err & 0x1)) //i bit on vifcode and not masked by VIF1_ERR
			{
				VIF_LOG("Interrupt on VIFcmd: %x (INTC_MASK = %x)", vif1.cmd, psHu32(INTC_MASK));

				++vif1.irq;

				if (istag && vif1.tag.size <= vif1.vifpacketsize) vif1.stallontag = true;

				if (vif1.tag.size == 0) break;
			}
		}

		if(!vif1.cmd) vif1Regs->stat &= ~VIF1_STAT_VPS_D;

		if((vif1Regs->stat & VIF1_STAT_VGW) || vif1.vifstalled == true) break;
	} // End of Transfer loop

	transferred += size - vif1.vifpacketsize;
	g_vifCycles += (transferred >> 2) * BIAS; /* guessing */
	vif1.irqoffset = transferred % 4; // cannot lose the offset

	if (vif1.irq && vif1.cmd == 0)
	{
		vif1.vifstalled = true;

		if (((vif1Regs->code >> 24) & 0x7f) != 0x7) vif1Regs->stat |= VIF1_STAT_VIS; // Note: commenting this out fixes WALL-E

		if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1))
			vif1.inprogress &= ~0x1;
		
		// spiderman doesn't break on qw boundaries
		if (istag) return -2;

		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;

		if ((vif1ch->qwc == 0) && (vif1.irqoffset == 0)) vif1.inprogress = 0;
		//Console::WriteLn("Stall on vif1, FromSPR = %x, Vif1MADR = %x Sif0MADR = %x STADR = %x", params psHu32(0x1000d010), vif1ch->madr, psHu32(0x1000c010), psHu32(DMAC_STADR));
		return -2;
	}

	vif1Regs->stat &= ~VIF1_STAT_VPS; //Vif goes idle as the stall happened between commands;
	if (vif1.cmd) vif1Regs->stat |= VIF1_STAT_VPS_W;  //Otherwise we wait for the data

	if (!istag)
	{
		transferred = transferred >> 2;
		vif1ch->madr += (transferred << 4);
		vif1ch->qwc -= transferred;		
	}
	
	if(vif1Regs->stat & VIF1_STAT_VGW)
	{
		vif1.vifstalled = true;
	}
	

	if (vif1ch->qwc == 0 && (vif1.irqoffset == 0 || istag == 1))
		vif1.inprogress &= ~0x1;

	return vif1.vifstalled ? -2 : 0;
}

void vif1TransferFromMemory()
{
	int size;
	u64* pMem = (u64*)dmaGetAddr(vif1ch->madr);

	// VIF from gsMemory
	if (pMem == NULL)  						//Is vif0ptag empty?
	{
		Console::WriteLn("Vif1 Tag BUSERR");
		psHu32(DMAC_STAT) |= 1 << 15;       //If yes, set BEIS (BUSERR) in DMAC_STAT register
		vif1.done = true;
		vif1Regs->stat &= ~0x1f000000;
		vif1ch->qwc = 0;
		CPU_INT(1, 0);

		return;						   //Return -1 as an error has occurred
	}

	// MTGS concerns:  The MTGS is inherently disagreeable with the idea of downloading
	// stuff from the GS.  The *only* way to handle this case safely is to flush the GS
	// completely and execute the transfer there-after.

	FreezeXMMRegs(1);
	
	if (GSreadFIFO2 == NULL)
	{
		for (size = vif1ch->qwc; size > 0; --size)
		{
			if (size > 1)
			{
				mtgsWaitGS();
				GSreadFIFO((u64*)&PS2MEM_HW[0x5000]);
			}
			pMem[0] = psHu64(0x5000);
			pMem[1] = psHu64(0x5008);
			pMem += 2;
		}
	}
	else
	{
		mtgsWaitGS();
		GSreadFIFO2(pMem, vif1ch->qwc);

		// set incase read
		psHu64(0x5000) = pMem[2*vif1ch->qwc-2];
		psHu64(0x5008) = pMem[2*vif1ch->qwc-1];
	}
	
	FreezeXMMRegs(0);
	
	g_vifCycles += vif1ch->qwc * 2;
	vif1ch->madr += vif1ch->qwc * 16; // mgs3 scene changes
	vif1ch->qwc = 0;
}

int  _VIF1chain()
{
	u32 *pMem;
	u32 ret;

	if (vif1ch->qwc == 0)
	{
		vif1.inprogress = 0;
		return 0;
	}

	if (vif1.dmamode == VIF_NORMAL_MEM_MODE)
	{
		vif1TransferFromMemory();
		vif1.inprogress = 0;
		return 0;
	}

	pMem = (u32*)dmaGetAddr(vif1ch->madr);
	if (pMem == NULL) 
	{
		vif1.cmd = 0;
		vif1.tag.size = 0;
		vif1ch->qwc = 0;
		return 0;
	}

	VIF_LOG("VIF1chain size=%d, madr=%lx, tadr=%lx",
	        vif1ch->qwc, vif1ch->madr, vif1ch->tadr);

	if (vif1.vifstalled)
		ret = VIF1transfer(pMem + vif1.irqoffset, vif1ch->qwc * 4 - vif1.irqoffset, 0);
	else
		ret = VIF1transfer(pMem, vif1ch->qwc * 4, 0);

	return ret;
}

bool _chainVIF1()
{
	return vif1.done;//Return Done
}

__forceinline void vif1SetupTransfer()
{
	switch (vif1.dmamode)
	{
		case VIF_NORMAL_MODE: //Normal
		case VIF_NORMAL_MEM_MODE: //Normal (From memory)
			vif1.inprogress = 1;
			vif1.done = true;
			g_vifCycles = 2;
			break;

		case VIF_CHAIN_MODE: //Chain
			int id;
			int ret;

			vif1ptag = (u32*)dmaGetAddr(vif1ch->tadr); //Set memory pointer to TADR
			if (vif1ptag == NULL)  						//Is vif0ptag empty?
			{
				Console::Error("Vif1 Tag BUSERR");
				vif1ch->chcr = (vif1ch->chcr & 0xFFFF) | ((*vif1ptag) & 0xFFFF0000);     //Transfer upper part of tag to CHCR bits 31-15
				psHu32(DMAC_STAT) |= 1 << 15;       //If yes, set BEIS (BUSERR) in DMAC_STAT register
				return;						   //Return -1 as an error has occurred
			}

			id = (vif1ptag[0] >> 28) & 0x7; //ID for DmaChain copied from bit 28 of the tag
			
			vif1ch->qwc  = (u16)vif1ptag[0];       //QWC set to lower 16bits of the tag
			vif1ch->madr = vif1ptag[1];            //MADR = ADDR field
			vif1ch->chcr = (vif1ch->chcr & 0xFFFF) | ((*vif1ptag) & 0xFFFF0000);     //Transfer upper part of tag to CHCR bits 31-15
			
			g_vifCycles += 1; // Add 1 g_vifCycles from the QW read for the tag

			// Transfer dma tag if tte is set

			VIF_LOG("VIF1 Tag %8.8x_%8.8x size=%d, id=%d, madr=%lx, tadr=%lx\n",
			        vif1ptag[1], vif1ptag[0], vif1ch->qwc, id, vif1ch->madr, vif1ch->tadr);

			if (!vif1.done && ((psHu32(DMAC_CTRL) & 0xC0) == 0x40) && (id == 4))   // STD == VIF1
			{
				// there are still bugs, need to also check if gif->madr +16*qwc >= stadr, if not, stall
				if ((vif1ch->madr + vif1ch->qwc * 16) >= psHu32(DMAC_STADR))
				{
					// stalled
					hwDmacIrq(13);
					return;
				}
			}

			vif1.inprogress = 1;

			if (vif1ch->chcr & 0x40)
			{

				if (vif1.vifstalled)
					ret = VIF1transfer(vif1ptag + (2 + vif1.irqoffset), 2 - vif1.irqoffset, 1);  //Transfer Tag on stall
				else
					ret = VIF1transfer(vif1ptag + 2, 2, 1);  //Transfer Tag

				if (ret < 0 && vif1.irqoffset < 2) 
				{
					vif1.inprogress = 0; //Better clear this so it has to do it again (Jak 1)
					return;       //There has been an error or an interrupt
				}
			}

			vif1.irqoffset = 0;
			vif1.done |= hwDmacSrcChainWithStack(vif1ch, id);

			if ((vif1ch->chcr & 0x80) && (vif1ptag[0] >> 31))  			       //Check TIE bit of CHCR and IRQ bit of tag
			{
				VIF_LOG("dmaIrq Set");

				vif1.done = true;
				return;												   //End Transfer
			}
			break;
	}
}

__forceinline void vif1Interrupt()
{
	VIF_LOG("vif1Interrupt: %8.8x", cpuRegs.cycle);

	g_vifCycles = 0;

	if(schedulepath3msk) Vif1MskPath3();

	if((vif1Regs->stat & VIF1_STAT_VGW))
	{
		if(gif->chcr & 0x100)
		{			
			CPU_INT(1, gif->qwc * BIAS);
			return;
		} 
		else vif1Regs->stat &= ~VIF1_STAT_VGW;
	
	}

	if ((vif1ch->chcr & 0x100) == 0) Console::WriteLn("Vif1 running when CHCR == %x", params vif1ch->chcr);

	if (vif1.irq && vif1.tag.size == 0)
	{
		vif1Regs->stat |= VIF1_STAT_INT;
		hwIntcIrq(VIF1intc);
		--vif1.irq;
		if (vif1Regs->stat & (VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
		{
			vif1Regs->stat &= ~0x1F000000; // FQC=0
			
			// One game doesnt like vif stalling at end, cant remember what. Spiderman isnt keen on it tho
			vif1ch->chcr &= ~0x100;
			return;
		}
		else if ((vif1ch->qwc > 0) || (vif1.irqoffset > 0))
		{
			if (vif1.stallontag)
				vif1SetupTransfer();
			else
				_VIF1chain();//CPU_INT(13, vif1ch->qwc * BIAS);
		}
	}

	if (vif1.inprogress) 
	{
		_VIF1chain();
		CPU_INT(1, g_vifCycles);
		return;
	}

	if ((!vif1.done) || (vif1.inprogress & 0x1))
	{

		if (!(psHu32(DMAC_CTRL) & 0x1))
		{
			Console::WriteLn("vif1 dma masked");
			return;
		}

		if ((vif1.inprogress & 0x1) == 0) vif1SetupTransfer();

		CPU_INT(1, g_vifCycles);
		return;
	}
	
	if (vif1.vifstalled && vif1.irq) 
	{
		CPU_INT(1, 0);
		return; //Dont want to end if vif is stalled.
	}
#ifdef PCSX2_DEVBUILD
	if (vif1ch->qwc > 0) Console::WriteLn("VIF1 Ending with %x QWC left");
	if (vif1.cmd != 0) Console::WriteLn("vif1.cmd still set %x tag size %x", params vif1.cmd, vif1.tag.size);
#endif

	vif1Regs->stat &= ~VIF1_STAT_VPS; //Vif goes idle as the stall happened between commands;
	vif1ch->chcr &= ~0x100;
	g_vifCycles = 0;
	hwDmacIrq(DMAC_VIF1);

	//Im not totally sure why Path3 Masking makes it want to see stuff in the fifo 
	//Games effected by setting, Fatal Frame, KH2, Shox, Crash N Burn, GT3/4 possibly
	//Im guessing due to the full gs fifo before the reverse? (Refraction)
	//Note also this is only the condition for reverse fifo mode, normal direction clears it as normal
	if(!vif1Regs->mskpath3 || (vif1ch->chcr & 0x1))vif1Regs->stat &= ~0x1F000000; // FQC=0
}

void dmaVIF1()
{
	VIF_LOG("dmaVIF1 chcr = %lx, madr = %lx, qwc  = %lx\n"
	        "        tadr = %lx, asr0 = %lx, asr1 = %lx",
	        vif1ch->chcr, vif1ch->madr, vif1ch->qwc,
	        vif1ch->tadr, vif1ch->asr0, vif1ch->asr1);

	g_vifCycles = 0;
	vif1.inprogress = 0;
	
	if (((psHu32(DMAC_CTRL) & 0xC) == 0x8))   // VIF MFIFO
	{
		//Console::WriteLn("VIFMFIFO\n");
		if (!(vif1ch->chcr & 0x4)) Console::WriteLn("MFIFO mode != Chain! %x", params vif1ch->chcr);
		vifMFIFOInterrupt();
		return;
	}

#ifdef PCSX2_DEVBUILD
	if ((psHu32(DMAC_CTRL) & 0xC0) == 0x40)   // STD == VIF1
	{
		//DevCon::WriteLn("VIF Stall Control Source = %x, Drain = %x", params (psHu32(0xe000) >> 4) & 0x3, (psHu32(0xe000) >> 6) & 0x3);
	}
#endif

	if (!(vif1ch->chcr & 0x4) || vif1ch->qwc > 0)   // Normal Mode
	{

		if ((psHu32(DMAC_CTRL) & 0xC0) == 0x40)
			Console::WriteLn("DMA Stall Control on VIF1 normal");

		if ((vif1ch->chcr & 0x1))  // to Memory
			vif1.dmamode = VIF_NORMAL_MODE;
		else
			vif1.dmamode = VIF_NORMAL_MEM_MODE;
	}
	else 
	{
		vif1.dmamode = VIF_CHAIN_MODE;
	}

	if(vif1.dmamode != VIF_NORMAL_MEM_MODE)
		vif1Regs->stat |= 0x10000000; // FQC=16
	else 
		vif1Regs->stat |= min((u16)16, vif1ch->qwc) << 24; // FQC=16

	// Chain Mode
	vif1.done = false;
	vif1Interrupt();
}

void vif1Write32(u32 mem, u32 value)
{
	switch (mem)
	{
		case VIF1_MARK:
			VIF_LOG("VIF1_MARK write32 0x%8.8x", value);

			/* Clear mark flag in VIF1_STAT and set mark with 'value' */
			vif1Regs->stat &= ~VIF1_STAT_MRK;
			vif1Regs->mark = value;
			break;
		
		case VIF1_FBRST:   // FBRST
			VIF_LOG("VIF1_FBRST write32 0x%8.8x", value);

			if (value & 0x1)
			{
				/* Reset VIF */
				memzero_obj(vif1);
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1ch->qwc = 0; //?
				psHu64(VIF1_FIFO) = 0;
				psHu64(0x10005008) = 0; // VIF1_FIFO + 8
				vif1.done = true;
				if(vif1Regs->mskpath3)
				{
					vif1Regs->mskpath3 = 0;
					psHu32(GIF_STAT) &= ~0x2;
					if(gif->chcr & 0x100) CPU_INT(2, 4);	
				}
				vif1Regs->err = 0;
				vif1.inprogress = 0;
				vif1Regs->stat &= ~(0x1F800000 | VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS | VIF1_STAT_VPS); // FQC=0
			}
			
			if (value & 0x2)
			{
				/* Force Break the VIF */
				/* I guess we should stop the VIF dma here, but not 100% sure (linuz) */
				vif1Regs->stat |= VIF1_STAT_VFS;
				vif1Regs->stat &= ~VIF1_STAT_VPS;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
				Console::WriteLn("vif1 force break");
			}
			
			if (value & 0x4)
			{
				/* Stop VIF */
				// Not completely sure about this, can't remember what game used this, but 'draining' the VIF helped it, instead of
				//   just stoppin the VIF (linuz).
				vif1Regs->stat |= VIF1_STAT_VSS;
				vif1Regs->stat &= ~VIF1_STAT_VPS;
				cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
				vif1.vifstalled = true;
			}
			
			if (value & 0x8)
			{
				bool cancel = false;

				/* Cancel stall, first check if there is a stall to cancel, and then clear VIF1_STAT VSS|VFS|VIS|INT|ER0|ER1 bits */
				if (vif1Regs->stat & (VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					cancel = true;
				}

				vif1Regs->stat &= ~(VIF1_STAT_VSS | VIF1_STAT_VFS | VIF1_STAT_VIS |
						VIF1_STAT_INT | VIF1_STAT_ER0 | VIF1_STAT_ER1);
				
				if (cancel)
				{
					if (vif1.vifstalled)
					{
						g_vifCycles = 0;
						// loop necessary for spiderman
						if ((psHu32(DMAC_CTRL) & 0xC) == 0x8)
						{
							//Console::WriteLn("MFIFO Stall");
							CPU_INT(10, vif1ch->qwc * BIAS);
						}
						else
						{
							// Gets the timing right - Flatout
							CPU_INT(1, vif1ch->qwc * BIAS);
						}
						vif1ch->chcr |= 0x100;
					}
				}
			}
			break;
			
		case VIF1_ERR:   // ERR
			VIF_LOG("VIF1_ERR write32 0x%8.8x", value);

			/* Set VIF1_ERR with 'value' */
			vif1Regs->err = value;
			break;
		
		case VIF1_STAT:   // STAT
			VIF_LOG("VIF1_STAT write32 0x%8.8x", value);

#ifdef PCSX2_DEVBUILD
			/* Only FDR bit is writable, so mask the rest */
			if ((vif1Regs->stat & VIF1_STAT_FDR) ^(value & VIF1_STAT_FDR))
			{
				// different so can't be stalled
				if (vif1Regs->stat & (VIF1_STAT_INT | VIF1_STAT_VSS | VIF1_STAT_VIS | VIF1_STAT_VFS))
				{
					DevCon::WriteLn("changing dir when vif1 fifo stalled");
				}
			}
#endif

			vif1Regs->stat = (vif1Regs->stat & ~VIF1_STAT_FDR) | (value & VIF1_STAT_FDR);
			if (vif1Regs->stat & VIF1_STAT_FDR)
			{
				vif1Regs->stat |= 0x01000000; // FQC=1 - hack but it checks this is true before tranfer? (fatal frame)
			}
			else
			{
				vif1ch->qwc = 0;
				vif1.vifstalled = false;
				vif1.done = true;
				vif1Regs->stat &= ~0x1F000000; // FQC=0
			}
			break;
			
		case VIF1_MODE:   // MODE
			vif1Regs->mode = value;
			break;
		
		case VIF1_R0:
		case VIF1_R1:
		case VIF1_R2:
		case VIF1_R3:
			assert((mem&0xf) == 0);
			g_vifRow1[(mem>>4) & 3] = value;
			break;
		
		case VIF1_C0:
		case VIF1_C1:
		case VIF1_C2:
		case VIF1_C3:
			assert((mem&0xf) == 0);
			g_vifCol1[(mem>>4) & 3] = value;
			break;
		
		default:
			Console::WriteLn("Unknown Vif1 write to %x", params mem);
			psHu32(mem) = value;
			break;
	}

	/* Other registers are read-only so do nothing for them */
}

void vif1Reset()
{
	/* Reset the whole VIF, meaning the internal pcsx2 vars, and all the registers */
	memzero_obj(vif1);
	memzero_obj(*vif1Regs);
	SetNewMask(g_vif1Masks, g_vif1HasMask3, 0, 0xffffffff);
	psHu64(VIF1_FIFO) = 0;
	psHu64(0x10005008) = 0; // VIF1_FIFO + 8
	vif1Regs->stat &= ~VIF1_STAT_VPS;
	vif1.done = true;
	cpuRegs.interrupt &= ~((1 << 1) | (1 << 10)); //Stop all vif1 DMA's
	vif1Regs->stat &= ~0x1F000000; // FQC=0
}

void SaveState::vif1Freeze()
{
	Freeze(vif1);

	Freeze(g_vif1HasMask3);
	Freeze(g_vif1Masks);
	Freeze(g_vifRow1);
	Freeze(g_vifCol1);
}
