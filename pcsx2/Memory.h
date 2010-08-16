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

#pragma once

#ifdef __LINUX__
#include <signal.h>
#endif

//#define ENABLECACHE
#include "vtlb.h"

#include <xmmintrin.h>

static __fi void CopyQWC( void* dest, const void* src )
{
	_mm_store_ps( (float*)dest, _mm_load_ps((const float*)src) );
}

static __fi void ZeroQWC( void* dest )
{
	_mm_store_ps( (float*)dest, _mm_setzero_ps() );
}

// Various useful locations
#define spr0 ((DMACh*)&eeMem->HW[0xD000])
#define spr1 ((DMACh*)&eeMem->HW[0xD400])

#define gif ((DMACh*)&eeMem->HW[0xA000])

#define vif0ch ((DMACh*)&eeMem->HW[0x8000])
#define vif1ch ((DMACh*)&eeMem->HW[0x9000])

#define sif0dma ((DMACh*)&eeMem->HW[0xc000])
#define sif1dma ((DMACh*)&eeMem->HW[0xc400])
#define sif2dma ((DMACh*)&eeMem->HW[0xc800])

#define ipu0dma ((DMACh *)&eeMem->HW[0xb000])
#define ipu1dma ((DMACh *)&eeMem->HW[0xb400])

#define PSM(mem)	(vtlb_GetPhyPtr((mem)&0x1fffffff)) //pcsx2 is a competition.The one with most hacks wins :D

#define psHs8(mem)	(*(s8 *)&eeMem->HW[(mem) & 0xffff])
#define psHs16(mem)	(*(s16*)&eeMem->HW[(mem) & 0xffff])
#define psHs32(mem)	(*(s32*)&eeMem->HW[(mem) & 0xffff])
#define psHs64(mem)	(*(s64*)&eeMem->HW[(mem) & 0xffff])
#define psHu8(mem)	(*(u8 *)&eeMem->HW[(mem) & 0xffff])
#define psHu16(mem)	(*(u16*)&eeMem->HW[(mem) & 0xffff])
#define psHu32(mem)	(*(u32*)&eeMem->HW[(mem) & 0xffff])
#define psHu64(mem)	(*(u64*)&eeMem->HW[(mem) & 0xffff])
#define psHu128(mem)(*(u128*)&eeMem->HW[(mem) & 0xffff])

#define psMs8(mem)	(*(s8 *)&eeMem->Main[(mem) & 0x1ffffff])
#define psMs16(mem)	(*(s16*)&eeMem->Main[(mem) & 0x1ffffff])
#define psMs32(mem)	(*(s32*)&eeMem->Main[(mem) & 0x1ffffff])
#define psMs64(mem)	(*(s64*)&eeMem->Main[(mem) & 0x1ffffff])
#define psMu8(mem)	(*(u8 *)&eeMem->Main[(mem) & 0x1ffffff])
#define psMu16(mem)	(*(u16*)&eeMem->Main[(mem) & 0x1ffffff])
#define psMu32(mem)	(*(u32*)&eeMem->Main[(mem) & 0x1ffffff])
#define psMu64(mem)	(*(u64*)&eeMem->Main[(mem) & 0x1ffffff])

#define psRs8(mem)	(*(s8 *)&eeMem->ROM[(mem) & 0x3fffff])
#define psRs16(mem)	(*(s16*)&eeMem->ROM[(mem) & 0x3fffff])
#define psRs32(mem)	(*(s32*)&eeMem->ROM[(mem) & 0x3fffff])
#define psRs64(mem)	(*(s64*)&eeMem->ROM[(mem) & 0x3fffff])
#define psRu8(mem)	(*(u8 *)&eeMem->ROM[(mem) & 0x3fffff])
#define psRu16(mem)	(*(u16*)&eeMem->ROM[(mem) & 0x3fffff])
#define psRu32(mem)	(*(u32*)&eeMem->ROM[(mem) & 0x3fffff])
#define psRu64(mem)	(*(u64*)&eeMem->ROM[(mem) & 0x3fffff])

#define psR1s8(mem)		(*(s8 *)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1s16(mem)	(*(s16*)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1s32(mem)	(*(s32*)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1s64(mem)	(*(s64*)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1u8(mem)		(*(u8 *)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1u16(mem)	(*(u16*)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1u32(mem)	(*(u32*)&eeMem->ROM1[(mem) & 0x3ffff])
#define psR1u64(mem)	(*(u64*)&eeMem->ROM1[(mem) & 0x3ffff])

#define psR2s8(mem)		(*(s8 *)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2s16(mem)	(*(s16*)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2s32(mem)	(*(s32*)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2s64(mem)	(*(s64*)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2u8(mem)		(*(u8 *)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2u16(mem)	(*(u16*)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2u32(mem)	(*(u32*)&eeMem->ROM2[(mem) & 0x3ffff])
#define psR2u64(mem)	(*(u64*)&eeMem->ROM2[(mem) & 0x3ffff])

#define psERs8(mem)		(*(s8 *)&eeMem->EROM[(mem) & 0x3ffff])
#define psERs16(mem)	(*(s16*)&eeMem->EROM[(mem) & 0x3ffff])
#define psERs32(mem)	(*(s32*)&eeMem->EROM[(mem) & 0x3ffff])
#define psERs64(mem)	(*(s64*)&eeMem->EROM[(mem) & 0x3ffff])
#define psERu8(mem)		(*(u8 *)&eeMem->EROM[(mem) & 0x3ffff])
#define psERu16(mem)	(*(u16*)&eeMem->EROM[(mem) & 0x3ffff])
#define psERu32(mem)	(*(u32*)&eeMem->EROM[(mem) & 0x3ffff])
#define psERu64(mem)	(*(u64*)&eeMem->EROM[(mem) & 0x3ffff])

#define psSs32(mem)		(*(s32 *)&eeMem->Scratch[(mem) & 0x3fff])
#define psSs64(mem)		(*(s64 *)&eeMem->Scratch[(mem) & 0x3fff])
#define psSs128(mem)	(*(s128*)&eeMem->Scratch[(mem) & 0x3fff])
#define psSu32(mem)		(*(u32 *)&eeMem->Scratch[(mem) & 0x3fff])
#define psSu64(mem)		(*(u64 *)&eeMem->Scratch[(mem) & 0x3fff])
#define psSu128(mem)	(*(u128*)&eeMem->Scratch[(mem) & 0x3fff])

#define psH_DMACh(mem)	(*(DMACh*)&eeMem->HW[(mem) & 0xffff])

extern void memAlloc();
extern void memReset();		// clears PS2 ram and loads the bios.  Throws Exception::FileNotFound on error.
extern void memShutdown();
extern void memSetKernelMode();
//extern void memSetSupervisorMode();
extern void memSetUserMode();
extern void memSetPageAddr(u32 vaddr, u32 paddr);
extern void memClearPageAddr(u32 vaddr);
extern void memBindConditionalHandlers();

extern void memMapVUmicro();

extern int mmap_GetRamPageInfo( u32 paddr );
extern void mmap_MarkCountedRamPage( u32 paddr );
extern void mmap_ResetBlockTracking();

#define memRead8 vtlb_memRead<mem8_t>
#define memRead16 vtlb_memRead<mem16_t>
#define memRead32 vtlb_memRead<mem32_t>
#define memRead64 vtlb_memRead64
#define memRead128 vtlb_memRead128

#define memWrite8 vtlb_memWrite<mem8_t>
#define memWrite16 vtlb_memWrite<mem16_t>
#define memWrite32 vtlb_memWrite<mem32_t>
#define memWrite64 vtlb_memWrite64
#define memWrite128 vtlb_memWrite128

extern u16 ba0R16(u32 mem);
