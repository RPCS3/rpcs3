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

namespace Ps2MemSize
{
	static const uint Base	= 0x02000000;		// 32 MB main memory!
	static const uint Rom	= 0x00400000;		// 4 MB main rom
	static const uint Rom1	= 0x00040000;		// DVD player
	static const uint Rom2	= 0x00080000;		// Chinese rom extension (?)
	static const uint ERom	= 0x001C0000;		// DVD player extensions (?)
	static const uint Hardware = 0x00010000;
	static const uint Scratch = 0x00004000;

	static const uint IopRam = 0x00200000;		// 2MB main ram on the IOP.
	static const uint IopHardware = 0x00010000;

	static const uint GSregs = 0x00002000;		// 8k for the GS registers and stuff.
}

typedef u8 mem8_t;
typedef u16 mem16_t;
typedef u32 mem32_t;
typedef u64 mem64_t;
typedef u128 mem128_t;


// --------------------------------------------------------------------------------------
//  Future-Planned VTLB pagefault scheme!
// --------------------------------------------------------------------------------------
// When enabled, the VTLB will use a large-area reserved memory range of 512megs for EE
// physical ram/rom access.  The base ram will be committed at 0x00000000, and ROMs will be
// at 0x1fc00000, etc.  All memory ranges in between will be uncommitted memory -- which
// means that the memory will *not* count against the operating system's physical memory
// pool.
//
// When the VTLB generates memory operations (loads/stores), it will assume that the op
// is addressing either RAM or ROM, and by assuming that it can generate a completely efficient
// direct memory access (one AND and one MOV instruction).  If the access is to another area of
// memory, such as hardware registers or scratchpad, the access will generate a page fault, the
// compiled block will be cleared and re-compiled using "full" VTLB translation logic.
//
#define VTLB_UsePageFaulting 0

#if VTLB_UsePageFaulting

// The order of the components in this struct *matter* -- it has been laid out so that the
// full breadth of PS2 RAM and ROM mappings are directly supported.
struct EEVM_MemoryAllocMess
{
	u8 (&Main)[Ps2MemSize::Base];				// Main memory (hard-wired to 32MB)

	u8 _padding1[0x1e000000-Ps2MemSize::Base]
	u8 (&ROM1)[Ps2MemSize::Rom1];				// DVD player

	u8 _padding2[0x1e040000-(0x1e000000+Ps2MemSize::Rom1)]
	u8 (&EROM)[Ps2MemSize::ERom];				// DVD player extensions

	u8 _padding3[0x1e400000-(0x1e040000+Ps2MemSize::EROM)]
	u8 (&ROM2)[Ps2MemSize::Rom2];				// Chinese extensions

	u8 _padding4[0x1fc00000-(0x1e040000+Ps2MemSize::Rom2)];
	u8 (&ROM)[Ps2MemSize::Rom];				// Boot rom (4MB)
};

#else

struct EEVM_MemoryAllocMess
{
	u8 Scratch[Ps2MemSize::Scratch];		// Scratchpad!
	u8 Main[Ps2MemSize::Base];				// Main memory (hard-wired to 32MB)
	u8 ROM[Ps2MemSize::Rom];				// Boot rom (4MB)
	u8 ROM1[Ps2MemSize::Rom1];				// DVD player
	u8 ROM2[Ps2MemSize::Rom2];				// Chinese extensions
	u8 EROM[Ps2MemSize::ERom];				// DVD player extensions

	// Two 1 megabyte (max DMA) buffers for reading and writing to high memory (>32MB).
	// Such accesses are not documented as causing bus errors but as the memory does
	// not exist, reads should continue to return 0 and writes should be discarded.
	// Probably.

	u8 ZeroRead[_1mb];
	u8 ZeroWrite[_1mb];
};

#endif

struct IopVM_MemoryAllocMess
{
	u8 Main[Ps2MemSize::IopRam];			// Main memory (hard-wired to 2MB)
	u8 P[0x00010000];						// I really have no idea what this is... --air
	u8 Sif[0x100];							// a few special SIF/SBUS registers (likely not needed)
};


// DevNote: EE and IOP hardware registers are done as a static array instead of a pointer in
// order to allow for simpler macros and reference handles to be defined  (we can safely use
// compile-time references to registers instead of having to use instance variables).

extern __pagealigned u8 eeHw[Ps2MemSize::Hardware];
extern __pagealigned u8 iopHw[Ps2MemSize::IopHardware];


extern EEVM_MemoryAllocMess* eeMem;
extern IopVM_MemoryAllocMess* iopMem;
