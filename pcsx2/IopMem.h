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


// Obtains a write-safe pointer into the IOP's memory, with TLB address translation.
// Hacky!  This should really never be used, since anything reading or writing through the
// TLB should be using iopMemRead/Write instead.
template<typename T>
static __forceinline T* iopVirtMemW( u32 mem )
{
	return (psxMemWLUT[(mem) >> 16] == 0) ? NULL : (T*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff));
}

// Obtains a read-safe pointer into the IOP's memory, with TLB address translation.
// Hacky!  This should really never be used, since anything reading or writing through the
// TLB should be using iopMemRead/Write instead.
template<typename T>
static __forceinline const T* iopVirtMemR( u32 mem )
{
	return (psxMemRLUT[(mem) >> 16] == 0) ? NULL : (const T*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff));
	//return ((const T*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)));
}

// Obtains a pointer to the IOP's physical mapping (bypasses the TLB)
static __forceinline u8* iopPhysMem( u32 addr )
{
	return &psxM[addr & 0x1fffff];
}

#define psxSs8(mem)		psxS[(mem) & 0xffff]
#define psxSs16(mem)	(*(s16*)&psxS[(mem) & 0xffff])
#define psxSs32(mem)	(*(s32*)&psxS[(mem) & 0xffff])
#define psxSu8(mem)		(*(u8*) &psxS[(mem) & 0xffff])
#define psxSu16(mem)	(*(u16*)&psxS[(mem) & 0xffff])
#define psxSu32(mem)	(*(u32*)&psxS[(mem) & 0xffff])

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

//#define PSXMs8(mem)  (*(s8 *)_PSXM(mem))
//#define PSXMs16(mem) (*(s16*)_PSXM(mem))
//#define PSXMs32(mem) (*(s32*)_PSXM(mem))
//#define PSXMu8(mem)  (*(u8 *)_PSXM(mem))
//#define PSXMu16(mem) (*(u16*)_PSXM(mem))
//#define PSXMu32(mem) (*(u32*)_PSXM(mem))

void psxMemAlloc();
void psxMemReset();
void psxMemShutdown();

u8   iopMemRead8 (u32 mem);
u16  iopMemRead16(u32 mem);
u32  iopMemRead32(u32 mem);
void iopMemWrite8 (u32 mem, u8 value);
void iopMemWrite16(u32 mem, u16 value);
void iopMemWrite32(u32 mem, u32 value);

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
