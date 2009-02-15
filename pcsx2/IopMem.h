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

#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__

extern u8 *psxM;
extern u8 *psxP;
extern u8 *psxH;
extern u8 *psxS;
extern uptr *psxMemWLUT;
extern const uptr *psxMemRLUT;

#ifdef TLB_DEBUG_MEM
void* PSXM(u32 mem);
void* _PSXM(u32 mem);
#else
#define PSXM(mem) (psxMemRLUT[(mem) >> 16] == 0 ? NULL : (void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
#define _PSXM(mem) ((const void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)))
#endif

#define psxSs8(mem)		psxS[(mem) & 0xffff]
#define psxSs16(mem)	(*(s16*)&psxS[(mem) & 0xffff])
#define psxSs32(mem)	(*(s32*)&psxS[(mem) & 0xffff])
#define psxSu8(mem)		(*(u8*) &psxS[(mem) & 0xffff])
#define psxSu16(mem)	(*(u16*)&psxS[(mem) & 0xffff])
#define psxSu32(mem)	(*(u32*)&psxS[(mem) & 0xffff])

#define psxMs8(mem)		psxM[(mem) & 0x1fffff]
#define psxMs16(mem)	(*(s16*)&psxM[(mem) & 0x1fffff])
#define psxMs32(mem)	(*(s32*)&psxM[(mem) & 0x1fffff])
#define psxMu8(mem)		(*(u8*) &psxM[(mem) & 0x1fffff])
#define psxMu16(mem)	(*(u16*)&psxM[(mem) & 0x1fffff])
#define psxMu32(mem)	(*(u32*)&psxM[(mem) & 0x1fffff])
#define psxMu64(mem)	(*(u64*)&psxM[(mem) & 0x1fffff])

#define psxPs8(mem)		psxP[(mem) & 0xffff]
#define psxPs16(mem)	(*(s16*)&psxP[(mem) & 0xffff])
#define psxPs32(mem)	(*(s32*)&psxP[(mem) & 0xffff])
#define psxPu8(mem)		(*(u8*) &psxP[(mem) & 0xffff])
#define psxPu16(mem)	(*(u16*)&psxP[(mem) & 0xffff])
#define psxPu32(mem)	(*(u32*)&psxP[(mem) & 0xffff])

#define psxHs8(mem)		psxH[(mem) & 0xffff]
#define psxHs16(mem)	(*(s16*)&psxH[(mem) & 0xffff])
#define psxHs32(mem)	(*(s32*)&psxH[(mem) & 0xffff])
#define psxHu8(mem)		(*(u8*) &psxH[(mem) & 0xffff])
#define psxHu16(mem)	(*(u16*)&psxH[(mem) & 0xffff])
#define psxHu32(mem)	(*(u32*)&psxH[(mem) & 0xffff])

#define PSXMs8(mem)  (*(s8 *)_PSXM(mem))
#define PSXMs16(mem) (*(s16*)_PSXM(mem))
#define PSXMs32(mem) (*(s32*)_PSXM(mem))
#define PSXMu8(mem)  (*(u8 *)_PSXM(mem))
#define PSXMu16(mem) (*(u16*)_PSXM(mem))
#define PSXMu32(mem) (*(u32*)_PSXM(mem))

void psxMemAlloc();
void psxMemReset();
void psxMemShutdown();

u8   psxMemRead8 (u32 mem);
u16  psxMemRead16(u32 mem);
u32  psxMemRead32(u32 mem);
void psxMemWrite8 (u32 mem, u8 value);
void psxMemWrite16(u32 mem, u16 value);
void psxMemWrite32(u32 mem, u32 value);

// x86reg and mmreg are always x86 regs
void psxRecMemRead8();
int psxRecMemConstRead8(u32 x86reg, u32 mem, u32 sign);

void psxRecMemRead16();
int psxRecMemConstRead16(u32 x86reg, u32 mem, u32 sign);

void psxRecMemRead32();
int psxRecMemConstRead32(u32 x86reg, u32 mem);

void psxRecMemWrite8();
int psxRecMemConstWrite8(u32 mem, int mmreg);

void psxRecMemWrite16();
int psxRecMemConstWrite16(u32 mem, int mmreg);

void psxRecMemWrite32();
int psxRecMemConstWrite32(u32 mem, int mmreg);

#endif /* __PSXMEMORY_H__ */
