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

#pragma once

#ifdef __LINUX__
#include <signal.h>
#endif

//#define ENABLECACHE

namespace Ps2MemSize
{
	static const uint Base	= 0x02000000;		// 32 MB main memory!
	static const uint Rom	= 0x00400000;		// 4 MB main rom
	static const uint Rom1	= 0x00040000;		// DVD player
	static const uint Rom2	= 0x00080000;		// Chinese rom extension (?)
	static const uint ERom	= 0x001C0000;		// DVD player extensions (?)
	static const uint Hardware = 0x00010000;
	static const uint Scratch = 0x00004000;

	static const uint IopRam = 0x00200000;	// 2MB main ram on the IOP.
	static const uint IopHardware = 0x00010000;

	static const uint GSregs = 0x00002000;		// 8k for the GS registers and stuff.
}

extern u8  *psM; //32mb Main Ram
extern u8  *psR; //4mb rom area
extern u8  *psR1; //256kb rom1 area (actually 196kb, but can't mask this)
extern u8  *psR2; // 0x00080000
extern u8  *psER; // 0x001C0000
extern u8  *psS; //0.015 mb, scratch pad

#define PS2MEM_BASE		psM
#define PS2MEM_HW		psH
#define PS2MEM_ROM		psR
#define PS2MEM_ROM1		psR1
#define PS2MEM_ROM2		psR2
#define PS2MEM_EROM		psER
#define PS2MEM_SCRATCH	psS

extern u8 g_RealGSMem[Ps2MemSize::GSregs];
#define PS2MEM_GS	g_RealGSMem

#define PSM(mem)	(vtlb_GetPhyPtr(mem&0x1fffffff)) //pcsx2 is a competition.The one with most hacks wins :D

#define psMs8(mem)	(*(s8 *)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs16(mem)	(*(s16*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs32(mem)	(*(s32*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMs64(mem)	(*(s64*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu8(mem)	(*(u8 *)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu16(mem)	(*(u16*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu32(mem)	(*(u32*)&PS2MEM_BASE[(mem) & 0x1ffffff])
#define psMu64(mem)	(*(u64*)&PS2MEM_BASE[(mem) & 0x1ffffff])

#define psRs8(mem)	(*(s8 *)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs16(mem)	(*(s16*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs32(mem)	(*(s32*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRs64(mem)	(*(s64*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu8(mem)	(*(u8 *)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu16(mem)	(*(u16*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu32(mem)	(*(u32*)&PS2MEM_ROM[(mem) & 0x3fffff])
#define psRu64(mem)	(*(u64*)&PS2MEM_ROM[(mem) & 0x3fffff])

#define psR1s8(mem)		(*(s8 *)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s16(mem)	(*(s16*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s32(mem)	(*(s32*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1s64(mem)	(*(s64*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u8(mem)		(*(u8 *)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u16(mem)	(*(u16*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u32(mem)	(*(u32*)&PS2MEM_ROM1[(mem) & 0x3ffff])
#define psR1u64(mem)	(*(u64*)&PS2MEM_ROM1[(mem) & 0x3ffff])

#define psR2s8(mem)		(*(s8 *)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s16(mem)	(*(s16*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s32(mem)	(*(s32*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2s64(mem)	(*(s64*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u8(mem)		(*(u8 *)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u16(mem)	(*(u16*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u32(mem)	(*(u32*)&PS2MEM_ROM2[(mem) & 0x3ffff])
#define psR2u64(mem)	(*(u64*)&PS2MEM_ROM2[(mem) & 0x3ffff])

#define psERs8(mem)		(*(s8 *)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs16(mem)	(*(s16*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs32(mem)	(*(s32*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERs64(mem)	(*(s64*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu8(mem)		(*(u8 *)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu16(mem)	(*(u16*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu32(mem)	(*(u32*)&PS2MEM_EROM[(mem) & 0x3ffff])
#define psERu64(mem)	(*(u64*)&PS2MEM_EROM[(mem) & 0x3ffff])

#define psSs8(mem)	(*(s8 *)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs16(mem)	(*(s16*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs32(mem)	(*(s32*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSs64(mem)	(*(s64*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu8(mem)	(*(u8 *)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu16(mem)	(*(u16*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu32(mem)	(*(u32*)&PS2MEM_SCRATCH[(mem) & 0x3fff])
#define psSu64(mem)	(*(u64*)&PS2MEM_SCRATCH[(mem) & 0x3fff])

//#define PSMs8(mem)	(*(s8 *)PSM(mem))
//#define PSMs16(mem)	(*(s16*)PSM(mem))
//#define PSMs32(mem)	(*(s32*)PSM(mem))
//#define PSMs64(mem)	(*(s64*)PSM(mem))
//#define PSMu8(mem)	(*(u8 *)PSM(mem))
//#define PSMu16(mem)	(*(u16*)PSM(mem))
//#define PSMu32(mem)	(*(u32*)PSM(mem))
//#define PSMu64(mem)	(*(u64*)PSM(mem))

extern void memAlloc();
extern void memReset();		// clears PS2 ram and loads the bios.  Throws Exception::FileNotFound on error.
extern void memShutdown();
extern void memSetKernelMode();
extern void memSetSupervisorMode();
extern void memSetUserMode();
extern void memSetPageAddr(u32 vaddr, u32 paddr);
extern void memClearPageAddr(u32 vaddr);

extern void memMapVUmicro();

#include "vtlb.h"

extern int mmap_GetRamPageInfo(void* ptr);
extern void mmap_MarkCountedRamPage(void* ptr,u32 vaddr);
extern void mmap_ResetBlockTracking();
extern void mmap_ClearCpuBlock( uint offset );

#define memRead8 vtlb_memRead8
#define memRead16 vtlb_memRead16
#define memRead32 vtlb_memRead32
#define memRead64 vtlb_memRead64
#define memRead128 vtlb_memRead128

#define memWrite8 vtlb_memWrite8
#define memWrite16 vtlb_memWrite16
#define memWrite32 vtlb_memWrite32
#define memWrite64 vtlb_memWrite64
#define memWrite128 vtlb_memWrite128

#define _eeReadConstMem8 0&&
#define _eeReadConstMem16 0&&
#define _eeReadConstMem32 0&&
#define _eeReadConstMem128 0&&
#define _eeWriteConstMem8 0&&
#define _eeWriteConstMem16 0&&
#define _eeWriteConstMem32 0&&
#define _eeWriteConstMem64 0&&
#define _eeWriteConstMem128 0&&
#define _eeMoveMMREGtoR 0&&

// extra ops
// These allow the old unused const versions of various HW accesses to continue to compile.
// (code left in for reference purposes, but is not needed by Vtlb)
#define _eeWriteConstMem16OP 0&&
#define _eeWriteConstMem32OP 0&&

#define recMemConstRead8 0&&
#define recMemConstRead16 0&&
#define recMemConstRead32 0&&
#define recMemConstRead64 0&&
#define recMemConstRead128 0&&

#define recMemConstWrite8 0&&
#define recMemConstWrite16 0&&
#define recMemConstWrite32 0&&
#define recMemConstWrite64 0&&
#define recMemConstWrite128 0&&

extern void loadBiosRom( const char *ext, u8 *dest, long maxSize );
extern u16 ba0R16(u32 mem);
